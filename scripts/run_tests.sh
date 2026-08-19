#!/usr/bin/env bash
set -euo pipefail

bash scripts/build_debug.sh
ctest --test-dir build-debug --output-on-failure
