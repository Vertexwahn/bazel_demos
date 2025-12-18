#include <flatland/core/scene.h>
#include <flatland/shape/shape.h>
#include <flatland/canvas/svg_canvas.h>
#include <flatland/core/filesystem.h>

#include <iostream>

using namespace Flatland;

int main() {
      // Arrange
    auto scene = loadScene("tests/scenes/disks.flatland.xml");
    auto ray = scene->getCamera()->generateRay();

    // Act
    MediumEvent2f me;
    bool hit = scene->intersect(ray, me);

    std::cout << hit << std::endl;
}
