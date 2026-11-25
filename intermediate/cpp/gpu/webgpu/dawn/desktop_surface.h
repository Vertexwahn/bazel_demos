#pragma once

#include <webgpu/webgpu_cpp.h>

struct GLFWwindow;

wgpu::Surface createDesktopSurface(const wgpu::Instance& instance, GLFWwindow* window);
