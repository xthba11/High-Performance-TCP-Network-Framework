#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-Debug}"

if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
  echo "Usage: $0 [Debug|Release]" >&2
  exit 1
fi

build_dir="build-${BUILD_TYPE,,}"

cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build "$build_dir" --parallel
