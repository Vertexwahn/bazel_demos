<!--
SPDX-FileCopyrightText: 2026 Julian Amann <dev@vertexwahn.de>
SPDX-License-Identifier: Apache-2.0
-->

# Stale `_virtual_includes` headers after a dependency upgrade (Windows)

On Windows, upgrading a dependency can break an incremental build. If a header
was removed from a `cc_library` that uses `strip_include_prefix`, Bazel keeps
the old copy of that header in `bazel-out`. cl.exe still finds the old copy on
the include path, and the build fails until `bazel clean`. A fresh build of the
same sources succeeds.

## Environment

- Windows 11, Visual Studio 2026 (MSVC 14.51)
- Bazel 9.3.3 (see [.bazelversion](.bazelversion)), rules_cc 0.2.28

## The example

`//:main` depends on `@mylib`, a module that exists in two versions:

| | `mylib-1.0` | `mylib-2.0` |
| --- | --- | --- |
| `:mylib_hdrs` (`strip_include_prefix = "include"`) | `mylib.h`, `scan.h` | `mylib.h` |
| `:mylib_hdrs_private` (`strip_include_prefix = "private"`) | - | `scan.h` |
| `scan.h` | `void scan_reset(void);` | `void scan_reset(const char *buffer);` |

Both versions build fine on their own. A [`local_path_override`](MODULE.bazel)
stands in for a registry, so the upgrade is a one-line change. The repository
stays `@@mylib+` and the target labels stay the same, as with a BCR version bump.

## Steps to reproduce

Automated (Windows PowerShell or pwsh; uses a temporary output base and restores
`MODULE.bazel` afterwards; exit code 1 means reproduced):

```powershell
.\reproduce.ps1
```

By hand:

1. `bazel build //:main` - succeeds with mylib 1.0.
2. In [MODULE.bazel](MODULE.bazel), change `path = "mylib-1.0"` to `path = "mylib-2.0"`.
3. `bazel build //:main` - fails:

```
ERROR: .../external/mylib+/BUILD.bazel:23:11: Compiling src/scan.c failed: undeclared inclusion(s) in rule '@@mylib+//:mylib':
this rule is missing dependency declarations for the following files included by 'src/scan.c':
  'bazel-out/x64_windows-fastbuild/bin/_virtual_includes/d365fa5c/scan.h'
ERROR: .../external/mylib+/BUILD.bazel:23:11: Compiling src/mylib.c failed: (Exit 2): cl.exe failed: ...
external/mylib+/src/mylib.c(10): error C2197: 'void scan_reset(void)': too many arguments for call
```

Expected: the second build succeeds, as a build of mylib 2.0 in a new output
base does.

## What happens

- `cc_library` with `strip_include_prefix` copies its headers to
  `bazel-out/<config>/bin/_virtual_includes/<hash of the target label>/`.
  On Windows these are plain file copies.
- With mylib 1.0, `_virtual_includes/d365fa5c/` (`:mylib_hdrs`) receives
  `mylib.h` and `scan.h`.
- With mylib 2.0, `:mylib_hdrs` has the same label and therefore the same
  directory, but declares only `mylib.h`. No action creates `scan.h` there any
  more, so Bazel never deletes it. The new `scan.h` goes to
  `_virtual_includes/3534aaa0/` (`:mylib_hdrs_private`).
- `d365fa5c` comes first on the include path. Compile actions on Windows are not
  sandboxed, so cl.exe sees the stale `scan.h`:
  - `mylib.c` fails to compile against the 1.0 declaration (C2197);
  - `scan.c` compiles, but Bazel's `/showIncludes` check rejects the undeclared
    header.

On Linux and macOS, the sandbox exposes only declared inputs, so this should not
happen there. That was not verified with this example; with
`--spawn_strategy=local` the same failure is expected.

### Real-world case

nasm 2.16.03 -> 3.02 on the BCR. 3.02 moved `asm/stdscan.h`, `asm/eval.h` and
others from `@nasm//:nasm_hdrs_asm` to `:nasm_hdrs_asm_private`, and
`stdscan_reset()` gained a parameter. Incremental Windows CI builds failed with
`error C2197: "void stdscan_reset(void)": too many arguments`. The failure only
showed up once nasm had to be recompiled for another reason, sometimes days after
the upgrade. Until then all nasm actions were cache hits.

## Workarounds

- `bazel clean` (gives up the incremental build), or
- delete `bazel-out/*/bin/_virtual_includes` before building
  (`.\reproduce.ps1 -Prune`). Bazel recreates exactly the declared headers.
  Their contents are unchanged, so no compile action re-runs.

## Possible fixes

- rules_cc: declare each `_virtual_includes/<hash>` directory as a single
  directory output (`ctx.actions.declare_directory`) instead of one file per
  header. Bazel deletes a directory output before recreating it, so headers that
  are no longer declared cannot survive.
- Bazel: remove outputs of actions that no longer exist from `bazel-out`, at
  least for unsandboxed spawns.
