/*
 * SPDX-FileCopyrightText: 2022 Julian Amann <dev@vertexwahn.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <webgpu/webgpu_cpp.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#else
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "desktop_surface.h"
#endif

namespace {
wgpu::Instance instance;
wgpu::Adapter adapter;
wgpu::Device device;
wgpu::Surface surface;
wgpu::SurfaceConfiguration configuration;
wgpu::RenderPipeline pipeline;
wgpu::Buffer uniforms;
wgpu::BindGroup bindGroup;
bool failed = false;
bool ready = false;
uint32_t maxTextureDimension = 0;
#ifndef __EMSCRIPTEN__
GLFWwindow* window = nullptr;
#endif
const auto start = std::chrono::steady_clock::now();

std::string text(wgpu::StringView message) {
    if (!message.data) {
        return {};
    }
    return {message.data, message.length == WGPU_STRLEN ? std::strlen(message.data) : message.length};
}

void fail(const std::string& message) {
    std::cerr << message << '\n';
    failed = true;
#ifdef __EMSCRIPTEN__
    EM_ASM({
        document.getElementById('status').textContent = UTF8ToString($0);
    }, message.c_str());
#endif
}

constexpr char shaderCode[] = R"(
struct Uniforms {
    angle: f32,
    scaleX: f32,
    scaleY: f32,
    padding: f32,
};
@group(0) @binding(0) var<uniform> uniforms: Uniforms;
struct Vertex {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};
@vertex fn vertexMain(@builtin(vertex_index) index: u32) -> Vertex {
    let positions = array(vec2f(-0.8, -0.8), vec2f(0.8, -0.8), vec2f(0.0, 0.8));
    let colors = array(vec3f(0, 0, 1), vec3f(0, 1, 0), vec3f(1, 0, 0));
    let p = positions[index];
    let c = cos(uniforms.angle);
    let s = sin(uniforms.angle);
    let rotated = vec2f(c * p.x - s * p.y, s * p.x + c * p.y);
    var vertex: Vertex;
    vertex.position = vec4f(rotated * vec2f(uniforms.scaleX, uniforms.scaleY), 0, 1);
    vertex.color = colors[index];
    return vertex;
}
@fragment fn fragmentMain(vertex: Vertex) -> @location(0) vec4f {
    return vec4f(vertex.color, 1);
}
)";

void initializeRenderer() {
    wgpu::SurfaceCapabilities capabilities;
    if (surface.GetCapabilities(adapter, &capabilities) != wgpu::Status::Success ||
        capabilities.formatCount == 0 || capabilities.alphaModeCount == 0) {
        fail("The adapter cannot present to this surface.");
        return;
    }
    configuration.device = device;
    configuration.format = capabilities.formats[0];
    configuration.usage = wgpu::TextureUsage::RenderAttachment;
    configuration.presentMode = wgpu::PresentMode::Fifo;
    configuration.alphaMode = capabilities.alphaModes[0];
    wgpu::Limits limits;
    if (device.GetLimits(&limits) != wgpu::Status::Success) {
        fail("Querying the WebGPU device limits failed.");
        return;
    }
    maxTextureDimension = limits.maxTextureDimension2D;

    wgpu::ShaderSourceWGSL wgsl;
    wgsl.code = shaderCode;
    wgpu::ShaderModuleDescriptor shaderDescriptor;
    shaderDescriptor.nextInChain = &wgsl;
    auto shader = device.CreateShaderModule(&shaderDescriptor);

    wgpu::ColorTargetState target;
    target.format = configuration.format;
    wgpu::FragmentState fragment;
    fragment.module = shader;
    fragment.entryPoint = "fragmentMain";
    fragment.targetCount = 1;
    fragment.targets = &target;
    wgpu::RenderPipelineDescriptor descriptor;
    descriptor.vertex.module = shader;
    descriptor.vertex.entryPoint = "vertexMain";
    descriptor.fragment = &fragment;
    pipeline = device.CreateRenderPipeline(&descriptor);

    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.size = 4 * sizeof(float);
    bufferDescriptor.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniforms = device.CreateBuffer(&bufferDescriptor);
    wgpu::BindGroupEntry entry;
    entry.binding = 0;
    entry.buffer = uniforms;
    entry.size = bufferDescriptor.size;
    wgpu::BindGroupDescriptor bindDescriptor;
    bindDescriptor.layout = pipeline.GetBindGroupLayout(0);
    bindDescriptor.entryCount = 1;
    bindDescriptor.entries = &entry;
    bindGroup = device.CreateBindGroup(&bindDescriptor);
    ready = !failed;
}

void requestDevice() {
    wgpu::DeviceDescriptor descriptor;
    descriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType, wgpu::StringView message) {
            fail("WebGPU error: " + text(message));
        });
    descriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowProcessEvents,
        [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
            if (reason != wgpu::DeviceLostReason::Destroyed) {
                fail("WebGPU device lost: " + text(message));
            }
        });
    adapter.RequestDevice(
        &descriptor, wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::RequestDeviceStatus status, wgpu::Device result, wgpu::StringView message) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                fail("RequestDevice failed: " + text(message));
                return;
            }
            device = std::move(result);
            initializeRenderer();
        });
}

void render() {
    instance.ProcessEvents();
    if (failed || !ready) {
        return;
    }
    int width = 0;
    int height = 0;
#ifdef __EMSCRIPTEN__
    double cssWidth = 0;
    double cssHeight = 0;
    emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
    const double ratio = emscripten_get_device_pixel_ratio();
    width = static_cast<int>(cssWidth * ratio);
    height = static_cast<int>(cssHeight * ratio);
#else
    glfwGetFramebufferSize(window, &width, &height);
#endif
    if (width <= 0 || height <= 0) {
        return;
    }
    width = std::min(width, static_cast<int>(maxTextureDimension));
    height = std::min(height, static_cast<int>(maxTextureDimension));
    if (configuration.width != static_cast<uint32_t>(width) ||
        configuration.height != static_cast<uint32_t>(height)) {
        configuration.width = static_cast<uint32_t>(width);
        configuration.height = static_cast<uint32_t>(height);
#ifdef __EMSCRIPTEN__
        emscripten_set_canvas_element_size("#canvas", width, height);
#endif
        surface.Configure(&configuration);
    }

    wgpu::SurfaceTexture frame;
    surface.GetCurrentTexture(&frame);
    switch (frame.status) {
        case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
        case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
            break;
        case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
            return;
        case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
        case wgpu::SurfaceGetCurrentTextureStatus::Lost:
            surface.Configure(&configuration);
            return;
        default:
            fail("Acquiring the surface texture failed.");
            return;
    }
    if (!frame.texture) {
        fail("WebGPU returned an empty surface texture.");
        return;
    }
    const float seconds = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - start).count();
    const float shortest = static_cast<float>(std::min(width, height));
    const float values[] = {seconds * 0.3f, shortest / width, shortest / height, 0.0f};
    device.GetQueue().WriteBuffer(uniforms, 0, values, sizeof(values));

    wgpu::RenderPassColorAttachment attachment;
    attachment.view = frame.texture.CreateView();
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = {0.3, 0.3, 0.3, 1.0};
    wgpu::RenderPassDescriptor passDescriptor;
    passDescriptor.colorAttachmentCount = 1;
    passDescriptor.colorAttachments = &attachment;
    auto encoder = device.CreateCommandEncoder();
    auto pass = encoder.BeginRenderPass(&passDescriptor);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(3);
    pass.End();
    auto commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
#ifndef __EMSCRIPTEN__
    surface.Present();
#endif
}
}  // namespace

int main() {
#ifndef __EMSCRIPTEN__
    glfwSetErrorCallback([](int, const char* message) { fail(message); });
    if (!glfwInit()) {
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640, 480, "Dawn WebGPU triangle", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
#endif
    instance = wgpu::CreateInstance();
    if (!instance) {
        fail("Creating the WebGPU instance failed.");
        return 1;
    }
#ifdef __EMSCRIPTEN__
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector source;
    source.selector = "#canvas";
    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = &source;
    surface = instance.CreateSurface(&descriptor);
#else
    surface = createDesktopSurface(instance, window);
#endif
    if (!surface) {
        fail("Creating the WebGPU surface failed.");
        return 1;
    }
    wgpu::RequestAdapterOptions options;
    options.compatibleSurface = surface;
    instance.RequestAdapter(
        &options, wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::RequestAdapterStatus status, wgpu::Adapter result, wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                fail("RequestAdapter failed: " + text(message));
                return;
            }
            adapter = std::move(result);
            requestDevice();
        });
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop([] {
        render();
        if (failed) {
            emscripten_cancel_main_loop();
        }
    }, 0, true);
#else
    while (!failed && !glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
        if (!ready || glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.01);
        }
    }
    if (ready && configuration.width != 0) {
        surface.Unconfigure();
    }
    bindGroup = nullptr;
    uniforms = nullptr;
    pipeline = nullptr;
    configuration.device = nullptr;
    device = nullptr;
    surface = nullptr;
    adapter = nullptr;
    instance = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
    return failed ? 1 : 0;
}
