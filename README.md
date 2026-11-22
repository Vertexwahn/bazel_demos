<!--
SPDX-FileCopyrightText: 2022-2026 Julian Amann <dev@vertexwahn.de>
SPDX-License-Identifier: Apache-2.0
-->

[![Support Ukraine](
https://img.shields.io/badge/Support-Ukraine-005BBB?labelColor=FFD500)](https://novaukraine.org/)

# Bazel Demos

[Bazel](https://bazel.build/) is a build system.
In this repository,
I store some demos, experiments, 
and playgrounds that show how to use Bazel with different programming languages, 
libraries, and tools.
Besides this, 
you can find misc experiments related to Bazel here.
Most of the things you find here are not production-ready but can serve as a starting point for your own developments.

## Organization of this repo

This repository is mainly organized into three parts:

- [core_principles](/core_principles): Core principles of Bazel with no focus on a specific programming language
- [hello_world](/hello_world): Plain Hello World demos in different programming languages supported by the Bazel build system
- [intermediate](/intermediate): Usage of different third-party dependencies and tools

Furthermore, the [.github/workflows](/.github/workflows) directory contains build jobs to ensure that specific demos work across different operating systems.
Not all demos that can be found here are also covered by CI.

### Hello World Demos

Under [hello_world](/hello_world), 
you can find Hello World examples in miscellaneous programming languages using Bazel.
The following table shows which examples the current CI matrix covers.
A dash means the example is not tested on that OS.

| Language support on  | macOS 26 | Ubuntu 26.04 | Windows Server 2025 |
| -------------------- | -------- | ------------ | ------------------- |
| C++                  | yes      | yes          | yes                 |
| C#                   | —        | yes          | —                   |
| D                    | —        | yes          | —                   |
| Erlang               | —        | yes          | —                   |
| Fortran              | —        | yes          | —                   |
| Gleam                | —        | yes          | —                   |
| Go                   | yes      | yes          | yes                 |
| Haskell              | —        | yes          | —                   |
| Java                 | yes      | yes          | yes                 |
| JavaScript           | yes      | yes          | yes                 |
| Julia                | —        | yes          | —                   |
| LaTeX                | yes      | yes          | —                   |
| Perl                 | yes      | yes          | yes                 |
| Python               | yes      | yes          | yes                 |
| R                    | —        | yes          | —                   |
| Rust                 | yes      | yes          | —                   |
| Shell                | yes      | yes          | —                   |
| Swift                | —        | yes          | —                   |

Pro tip: Ask you favorite AI to come up with an example in your desired language and platform.  

### More complex examples

- [Vertexwahn/LichtFeld-Studio-bazel](https://github.com/Vertexwahn/LichtFeld-Studio-bazel): [LichtFeld Studio](https://lichtfeld.io/) build with Bazel
- [Vertexwahn/pbrt-v4-bazel](https://github.com/Vertexwahn/pbrt-v4-bazel): [pbrt, Version 4](https://github.com/mmp/pbrt-v4) build with Bazel

## Similar projects

Here you can find a list of other repositories with Bazel demos:

- [aspect-build/bazel-examples](https://github.com/aspect-build/bazel-examples)
- [bazelbuild/codelabs](https://github.com/bazelbuild/codelabs)
- [bazelbuild/examples](https://github.com/bazelbuild/examples)
- [clchiou/bazel-examples](https://github.com/clchiou/bazel-examples)
- [FaBrand/bazel-learning](https://github.com/FaBrand/bazel-learning)
- [kriscfoster/multi-language-bazel-monorepo](https://github.com/kriscfoster/multi-language-bazel-monorepo)
- [limdor/bazel-examples](https://github.com/limdor/bazel-examples)
- [salesforce/bazel-demo](https://github.com/salesforce/bazel-demo)

## Contributions welcome

If you have a Bazel demo you want to share, feel free to open a pull request.
I am always happy about seeing new programming languages supported by Bazel or interesting usage of tools or third-party dependencies. 
Of course, improvements are also welcome.

## License

If not stated otherwise the license of the source files in this repository (BazelDemos) is under the Apache License 2.0.

## Credits

A big thanks goes to all the providers, developers, and maintainers of the Bazel universe! :heart:
