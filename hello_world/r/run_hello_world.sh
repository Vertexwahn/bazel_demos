#!/usr/bin/env bash
set -euo pipefail

runfiles_dir="${RUNFILES_DIR:-${0}.runfiles}"
exec Rscript "${runfiles_dir}/_main/HelloWorld.R"
