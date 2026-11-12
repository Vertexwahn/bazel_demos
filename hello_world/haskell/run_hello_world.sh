#!/usr/bin/env bash
set -euo pipefail

exec runghc "${RUNFILES_DIR:-${0}.runfiles}/_main/HelloWorld.hs"
