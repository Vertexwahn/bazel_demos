// OCCT bottle tutorial (https://occt3d.com/dev/doc/overview/html/occt__tutorial.html),
// extended with a Wavefront OBJ export so the result can be opened in Blender.

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeSegment.hxx>
#include <GC_MakeSegment2d.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <NCollection_List.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

TopoDS_Shape MakeBottle(double theWidth, double theHeight,
                        double theThickness)
{
    // Profile : Define Support Points
    gp_Pnt aPnt1(-theWidth / 2., 0, 0);        
    gp_Pnt aPnt2(-theWidth / 2., -theThickness / 4., 0);
    gp_Pnt aPnt3(0, -theThickness / 2., 0);
    gp_Pnt aPnt4(theWidth / 2., -theThickness / 4., 0);
    gp_Pnt aPnt5(theWidth / 2., 0, 0);

    // Profile : Define the Geometry
    occ::handle<Geom_TrimmedCurve> anArcOfCircle = GC_MakeArcOfCircle(aPnt2,aPnt3,aPnt4);
    occ::handle<Geom_TrimmedCurve> aSegment1 = GC_MakeSegment(aPnt1, aPnt2);
    occ::handle<Geom_TrimmedCurve> aSegment2 = GC_MakeSegment(aPnt4, aPnt5);

    // Profile : Define the Topology
    TopoDS_Edge anEdge1 = BRepBuilderAPI_MakeEdge(aSegment1);
    TopoDS_Edge anEdge2 = BRepBuilderAPI_MakeEdge(anArcOfCircle);
    TopoDS_Edge anEdge3 = BRepBuilderAPI_MakeEdge(aSegment2);
    TopoDS_Wire aWire  = BRepBuilderAPI_MakeWire(anEdge1, anEdge2, anEdge3);

    // Complete Profile
    gp_Ax1 xAxis = gp::OX();
    gp_Trsf aTrsf;

    aTrsf.SetMirror(xAxis);
    BRepBuilderAPI_Transform aBRepTrsf(aWire, aTrsf);
    TopoDS_Shape aMirroredShape = aBRepTrsf.Shape();
    TopoDS_Wire aMirroredWire = TopoDS::Wire(aMirroredShape);

    BRepBuilderAPI_MakeWire aMkWire;
    aMkWire.Add(aWire);
    aMkWire.Add(aMirroredWire);
    TopoDS_Wire aWireProfile = aMkWire.Wire();

    // Body : Prism the Profile
    TopoDS_Face aFaceProfile = BRepBuilderAPI_MakeFace(aWireProfile);
    gp_Vec aPrismVec(0, 0, theHeight);
    TopoDS_Shape aBody = BRepPrimAPI_MakePrism(aFaceProfile, aPrismVec);

    // Body : Apply Fillets
    BRepFilletAPI_MakeFillet aMkFillet(aBody);
    TopExp_Explorer anEdgeExplorer(aBody, TopAbs_EDGE);
    while(anEdgeExplorer.More()){
        TopoDS_Edge anEdge = TopoDS::Edge(anEdgeExplorer.Current());
        //Add edge to fillet algorithm
        aMkFillet.Add(theThickness / 12., anEdge);
        anEdgeExplorer.Next();
    }

    aBody = aMkFillet.Shape();

    // Body : Add the Neck
    gp_Pnt aNeckLocation(0, 0, theHeight);
    gp_Dir aNeckAxis = gp::DZ();
    gp_Ax2 neckAx2(aNeckLocation, aNeckAxis);

    double aNeckRadius = theThickness / 4.;
    double aNeckHeight = theHeight / 10.;

    BRepPrimAPI_MakeCylinder aMkCylinder(neckAx2, aNeckRadius, aNeckHeight);
    TopoDS_Shape aNeck = aMkCylinder.Shape();

    BRepAlgoAPI_Fuse aFuser(aBody, aNeck);
    if (aFuser.IsDone())
    {
      aBody = aFuser.Shape();
    }
    else
    {
      // Fuse failed; proceed with unmodified aBody.
      // In production code, report the error and handle the failure appropriately.
    }

    // Body : Create a Hollowed Solid
    TopoDS_Face   aFaceToRemove;
    double aZMax = -1;

    for(TopExp_Explorer aFaceExplorer(aBody, TopAbs_FACE); aFaceExplorer.More(); aFaceExplorer.Next()){
        TopoDS_Face aFace = TopoDS::Face(aFaceExplorer.Current());
        // Check if <aFace> is the top face of the bottle's neck 
        occ::handle<Geom_Surface> aSurface = BRep_Tool::Surface(aFace);
        if(!aSurface.IsNull() && aSurface->DynamicType() == STANDARD_TYPE(Geom_Plane)){
            occ::handle<Geom_Plane> aPlane = occ::down_cast<Geom_Plane>(aSurface);
            if (!aPlane.IsNull())
            {
              gp_Pnt aPnt = aPlane->Location();
              double aZ   = aPnt.Z();
              if(aZ > aZMax){
                  aZMax = aZ;
                  aFaceToRemove = aFace;
              }
            }
        }
    }

    NCollection_List<TopoDS_Shape> aFacesToRemove;
    aFacesToRemove.Append(aFaceToRemove);
    BRepOffsetAPI_MakeThickSolid aSolidMaker;
    aSolidMaker.MakeThickSolidByJoin(aBody, aFacesToRemove, -theThickness / 50, 1.e-3);
    aBody = aSolidMaker.Shape();
    // Threading : Create Surfaces
    occ::handle<Geom_CylindricalSurface> aCyl1 = new Geom_CylindricalSurface(neckAx2, aNeckRadius * 0.99);
    occ::handle<Geom_CylindricalSurface> aCyl2 = new Geom_CylindricalSurface(neckAx2, aNeckRadius * 1.05);

    // Threading : Define 2D Curves
    gp_Pnt2d aPnt(2. * M_PI, aNeckHeight / 2.);
    gp_Dir2d aDir(2. * M_PI, aNeckHeight / 4.);
    gp_Ax2d anAx2d(aPnt, aDir);

    double aMajor = 2. * M_PI;
    double aMinor = aNeckHeight / 10;

    occ::handle<Geom2d_Ellipse> anEllipse1 = new Geom2d_Ellipse(anAx2d, aMajor, aMinor);
    occ::handle<Geom2d_Ellipse> anEllipse2 = new Geom2d_Ellipse(anAx2d, aMajor, aMinor / 4);
    occ::handle<Geom2d_TrimmedCurve> anArc1 = new Geom2d_TrimmedCurve(anEllipse1, 0, M_PI);
    occ::handle<Geom2d_TrimmedCurve> anArc2 = new Geom2d_TrimmedCurve(anEllipse2, 0, M_PI);
    gp_Pnt2d anEllipsePnt1 = anEllipse1->Value(0);
    gp_Pnt2d anEllipsePnt2 = anEllipse1->Value(M_PI);

    occ::handle<Geom2d_TrimmedCurve> aSegment = GC_MakeSegment2d(anEllipsePnt1, anEllipsePnt2);
    // Threading : Build Edges and Wires
    TopoDS_Edge anEdge1OnSurf1 = BRepBuilderAPI_MakeEdge(anArc1, aCyl1);
    TopoDS_Edge anEdge2OnSurf1 = BRepBuilderAPI_MakeEdge(aSegment, aCyl1);
    TopoDS_Edge anEdge1OnSurf2 = BRepBuilderAPI_MakeEdge(anArc2, aCyl2);
    TopoDS_Edge anEdge2OnSurf2 = BRepBuilderAPI_MakeEdge(aSegment, aCyl2);
    TopoDS_Wire aThreadingWire1 = BRepBuilderAPI_MakeWire(anEdge1OnSurf1, anEdge2OnSurf1);
    TopoDS_Wire aThreadingWire2 = BRepBuilderAPI_MakeWire(anEdge1OnSurf2, anEdge2OnSurf2);
    BRepLib::BuildCurves3d(aThreadingWire1);
    BRepLib::BuildCurves3d(aThreadingWire2);

    // Create Threading 
    BRepOffsetAPI_ThruSections aTool(true);
    aTool.AddWire(aThreadingWire1);
    aTool.AddWire(aThreadingWire2);
    aTool.CheckCompatibility(false);

    TopoDS_Shape aThreading = aTool.Shape();

    // Building the Resulting Compound 
    TopoDS_Compound aRes;
    BRep_Builder aBuilder;
    aBuilder.MakeCompound (aRes);
    aBuilder.Add (aRes, aBody);
    aBuilder.Add (aRes, aThreading);

    return aRes;
}

namespace
{
  // Tessellates the shape and dumps the triangulation as a Wavefront OBJ file.
  bool WriteObj(const TopoDS_Shape& theShape, const std::string& thePath,
                double theLinearDeflection, double theAngularDeflection)
  {
    BRepMesh_IncrementalMesh aMesher(theShape, theLinearDeflection, false,
                                     theAngularDeflection, true);
    if (!aMesher.IsDone())
    {
      std::cerr << "Meshing of the shape failed." << std::endl;
      return false;
    }

    std::ofstream aFile(thePath.c_str());
    if (!aFile)
    {
      std::cerr << "Cannot open '" << thePath << "' for writing." << std::endl;
      return false;
    }
    aFile.precision(9);
    aFile << "# OCCT bottle tutorial\n";
    aFile << "o Bottle\n";

    int aNodeOffset = 1;
    int aTriangleCount = 0;
    for (TopExp_Explorer aFaceExplorer(theShape, TopAbs_FACE); aFaceExplorer.More(); aFaceExplorer.Next())
    {
      const TopoDS_Face aFace = TopoDS::Face(aFaceExplorer.Current());
      TopLoc_Location aLocation;
      occ::handle<Poly_Triangulation> aTriangulation = BRep_Tool::Triangulation(aFace, aLocation);
      if (aTriangulation.IsNull() || aTriangulation->NbTriangles() == 0)
      {
        continue;
      }

      const gp_Trsf& aTrsf = aLocation.Transformation();
      for (int aNodeIdx = 1; aNodeIdx <= aTriangulation->NbNodes(); ++aNodeIdx)
      {
        const gp_Pnt aPnt = aTriangulation->Node(aNodeIdx).Transformed(aTrsf);
        aFile << "v " << aPnt.X() << ' ' << aPnt.Y() << ' ' << aPnt.Z() << '\n';
      }

      const bool isReversed = (aFace.Orientation() == TopAbs_REVERSED);
      for (int aTriIdx = 1; aTriIdx <= aTriangulation->NbTriangles(); ++aTriIdx)
      {
        int aN1 = 0, aN2 = 0, aN3 = 0;
        aTriangulation->Triangle(aTriIdx).Get(aN1, aN2, aN3);
        if (isReversed)
        {
          std::swap(aN2, aN3);
        }
        aFile << "f " << (aN1 + aNodeOffset - 1)
              << ' '  << (aN2 + aNodeOffset - 1)
              << ' '  << (aN3 + aNodeOffset - 1) << '\n';
        ++aTriangleCount;
      }

      aNodeOffset += aTriangulation->NbNodes();
    }

    if (aTriangleCount == 0)
    {
      std::cerr << "The shape produced no triangles." << std::endl;
      return false;
    }

    aFile.flush();
    if (!aFile)
    {
      std::cerr << "Writing '" << thePath << "' failed." << std::endl;
      return false;
    }

    std::cout << "Wrote " << aTriangleCount << " triangles to " << thePath << std::endl;
    return true;
  }
}

int main(int argc, char** argv)
{
  const std::string anOutputPath = (argc > 1) ? argv[1] : "bottle.obj";
  try
  {
    const TopoDS_Shape aBottle = MakeBottle(50.0, 70.0, 30.0);
    if (!WriteObj(aBottle, anOutputPath, 0.1, 0.3))
    {
      return 1;
    }
  }
  catch (const Standard_Failure& theFailure)
  {
    std::cerr << "OCCT error: " << theFailure.GetMessageString() << std::endl;
    return 1;
  }
  return 0;
}

