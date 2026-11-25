#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
bazel build //:index-wasm
python3 -m http.server 8000 --bind 127.0.0.1 --directory bazel-bin/index-wasm
