<!--
SPDX-FileCopyrightText: 2022-2026 Julian Amann <dev@vertexwahn.de>
SPDX-License-Identifier: Apache-2.0
-->

# Dawn / browser WebGPU triangle

The same C++20 renderer draws a rotating RGB triangle using native Dawn on the
desktop and browser WebGPU through Emscripten. Both paths use the current surface
API, asynchronous adapter/device requests, and WGSL.

## Prerequisites

- Bazel **9.3.3** (or Bazelisk, which reads `.bazelversion`).
- Python 3 on `PATH` to prepare the native Dawn source repository.
- Desktop: a C++20 compiler and a working GPU driver. On Windows, install Visual
  Studio's Desktop development with C++ workload and a Windows 10/11 SDK, plus Bash
  from Git for Windows or MSYS2 (`BAZEL_SH` must point to `bash.exe`).
- Linux desktop: X11 development packages (`libx11-dev`, `libxrandr-dev`,
  `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`) and a Vulkan-capable driver.
  This demo uses X11, including XWayland, rather than native Wayland.
- macOS desktop: Xcode command-line tools and Metal.
- Browser: WebGPU support and a secure context (`localhost` or HTTPS).

Run all commands from this directory. Dependencies are declared through Bzlmod
in [MODULE.bazel](MODULE.bazel); no WORKSPACE setup or local Qt/Vulkan SDK paths
are needed. The direct BCR dependencies are pinned to their latest non-yanked
versions as of 2026-11-22.

Native Dawn is not published in BCR. Its upstream `v20261120.121236` source
archive has a SHA-256 pin and a standalone [native Bazel overlay](bazel/dawn.BUILD.bazel).
All C++ compilation and API code generation use Bazel's C++ and Python rules:
there are no CMake, Ninja, `rules_foreign_cc`, or nested builds. Abseil, SPIR-V
headers, Vulkan headers/utilities, and GLFW come from BCR. Dawn's Python
generators use a Bazel-managed Python runtime and hash-pinned Jinja2/MarkupSafe
wheels. Tint's generated sources are already checked into the pinned upstream
revision. Only WGSL and the platform's native shader writer/backend are built;
tests, fuzzers, benchmarks, OpenGL, and Dawn wire support are not needed.
The first desktop build compiles Dawn/Tint and can take several minutes.

The browser uses BCR's `emdawnwebgpu` bindings, not the removed Emscripten
`USE_WEBGPU` setting or a port downloaded outside Bazel. Desktop and browser use
their respective matching headers/libraries; do not mix the two header versions.
The BCR bindings have a one-line patch adding their missing `<string>` include,
required by the newer libc++ shipped with Emscripten 7.

## Desktop

```shell
bazel run //:DawnDemoGlfw
```

`//:DawnDemo` is an alias for the GLFW target. The obsolete Qt renderer has been
retired. Windows uses D3D11/D3D12, macOS uses Metal, and Linux uses Vulkan.
The framebuffer and surface configuration follow window resizing; rendering
pauses while minimized.

## Browser

```shell
bazel build //:index-wasm
python -m http.server 8000 --bind 127.0.0.1 --directory bazel-bin/index-wasm
```

Open <http://localhost:8000/index.html>. On Linux/macOS, use `python3` if needed.
The build produces `index.html`, `index.js`, and `index.wasm` together under
`bazel-bin/index-wasm`; no manual copying or JavaScript device preinitialization
is needed. Browser initialization and validation failures are shown on the page
and in the developer console.

The Windows CI workflow builds both `//:DawnDemoGlfw` and `//:index-wasm`. It does
not require a GPU or attempt to open a desktop window on the runner.
Both targets have been built on Windows with Bazel 9.3.3, and triangle rendering
and resizing have been checked on native Windows and in a WebGPU browser.
The Linux and macOS desktop configurations have not been tested.

## Legal notes

The original demo was based on
[cwoffenden/hello-webgpu](https://github.com/cwoffenden/hello-webgpu) (CC0/public
domain). Modifications retain their source-file license notices.
[LICENSE.skia](LICENSE.skia) is retained for the historical Skia-derived build
files; those obsolete overlays have been removed.

## References

- <https://dawn.googlesource.com/dawn>
- <https://registry.bazel.build/modules/emdawnwebgpu>
- <https://eliemichel.github.io/LearnWebGPU/>
