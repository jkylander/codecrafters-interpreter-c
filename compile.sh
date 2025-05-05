#!/bin/sh
set -e # Exit early if any commands fail

cd "$(dirname "$0")"
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build ./build
