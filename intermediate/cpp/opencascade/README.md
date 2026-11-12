# OpenCASCADE "Make Bottle" Demo

Baut das [OCCT Bottle Tutorial](https://occt3d.com/dev/doc/overview/html/occt__tutorial.html)
mit Bazel (Bzlmod) und exportiert das Ergebnis als Wavefront-OBJ, das z. B. in Blender
geoeffnet werden kann.

## Bauen und ausfuehren

```powershell
# OBJ direkt als Build-Artefakt erzeugen -> bazel-bin/bottle.obj
bazel build //:bottle_obj

# oder das Programm selbst starten (Standardausgabe: bottle.obj im aktuellen Verzeichnis)
bazel run //:bottle -- C:\temp\bottle.obj
```

Der erste Build kompiliert OCCT komplett aus den Quellen (ca. 5300 Uebersetzungseinheiten)
und dauert entsprechend lange; danach greift der Bazel-Cache.

## Aufbau

| Datei | Inhalt |
| --- | --- |
| [main.cpp](main.cpp) | `MakeBottle()` aus dem Tutorial, Vernetzung via `BRepMesh_IncrementalMesh` und OBJ-Export |
| [MODULE.bazel](MODULE.bazel) | Bzlmod-Abhaengigkeiten und `http_archive` fuer OCCT V8_0_1 |
| [third_party/occt.BUILD](third_party/occt.BUILD) | BUILD-Datei fuer das OCCT-Repository (Modul-Bibliotheken) |
| [third_party/occt.bzl](third_party/occt.bzl) | Makros/Regel fuer flaches Include-Verzeichnis und Toolkit-Bibliotheken |

OCCT wird **ohne** `rules_foreign_cc`/CMake gebaut: reine `cc_library`-Targets pro OCCT-Modul
(FoundationClasses -> ModelingData -> ModelingAlgorithms). Da OCCT-Header ueber ihren blossen
Dateinamen inkludiert werden, spiegelt die Regel `_flat_headers` alle Header per Symlink in ein
einzelnes `inc/`-Verzeichnis; `Standard_Version.hxx` wird per `expand_template` aus dem
OCCT-Template erzeugt. Visualization, DataExchange, ApplicationFramework und Draw werden nicht
gebaut, da das Demo sie nicht benoetigt.

