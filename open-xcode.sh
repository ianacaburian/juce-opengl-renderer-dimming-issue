#! /bin/bash

set -e
cmake . -G Xcode -B build/xcode "$@"
cmake --open build/xcode