#!/usr/bin/env bash
set -euo pipefail

cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"
exec gleam run --target javascript
