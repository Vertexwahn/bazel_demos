#include "desktop_surface.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

wgpu::Surface createDesktopSurface(const wgpu::Instance& instance, GLFWwindow* window) {
    NSWindow* nativeWindow = glfwGetCocoaWindow(window);
    NSView* view = [nativeWindow contentView];
    [view setWantsLayer:YES];
    CAMetalLayer* layer = [CAMetalLayer layer];
    [layer setContentsScale:[nativeWindow backingScaleFactor]];
    [view setLayer:layer];
    wgpu::SurfaceSourceMetalLayer source;
    source.layer = layer;
    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &source;
    return instance.CreateSurface(&descriptor);
}
