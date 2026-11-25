#include "desktop_surface.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

wgpu::Surface createDesktopSurface(const wgpu::Instance& instance, GLFWwindow* window) {
#if defined(_WIN32)
    wgpu::SurfaceSourceWindowsHWND source;
    source.hinstance = GetModuleHandle(nullptr);
    source.hwnd = glfwGetWin32Window(window);
#else
    wgpu::SurfaceSourceXlibWindow source;
    source.display = glfwGetX11Display();
    source.window = glfwGetX11Window(window);
#endif
    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &source;
    return instance.CreateSurface(&descriptor);
}
