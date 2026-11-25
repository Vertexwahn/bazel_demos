"""Flatten upstream packages for the pinned, standalone Dawn/Tint overlay."""

from pathlib import Path


for directory in ("src/tint", "src/utils"):
    for path in Path(directory).rglob("BUILD.bazel"):
        path.unlink()
