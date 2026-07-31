#!/usr/bin/env bash

BUILD_DIR="./build"

# Generate build system files
gen() {
    cmake -S . -B ./build "$@"
}

# Build the project 
build() {
    cmake --build ./build "$@"
}

valgrind_run() {
  valgrind --leak-check=full --show-leak-kinds=all --show-reachable=yes --track-origins=yes --num-callers=20 --track-fds=yes --verbose ./build/ceeify "$@"
}

valgrind_test() {
  valgrind --leak-check=full --show-leak-kinds=all --show-reachable=yes --track-origins=yes --num-callers=20 --track-fds=yes --verbose ./build/test_ceeify
}

lint() {
  dir="$1"
  if [ -z "$dir" ]; then
    echo "Usage: f <directory>"
    return 1
  fi

  find "$dir" \( -name "*.c" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
}

analyze() {
  if [ ! -f ./build/compile_commands.json ]; then
    echo "compile_commands.json not found. Run: ./tasks.sh gen"
    return 1
  fi

  files=$(jq -r '.[].file' build/compile_commands.json | sort -u)

  clang-tidy \
    -p ./build \
    -quiet \
    -header-filter='^'$(pwd) \
    --config-file='.clang-tidy' \
    --use-color \
    --fix-errors \
    $files
}

ikos() {
  if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    set -euo pipefail
  fi

  if [[ -d "$BUILD_DIR" ]]; then
    echo "Removing existing build directory…"
    rm -rf -- "$BUILD_DIR"
  fi

  echo "Configuring project with IKOS…"
  echo "n" | ikos-scan cmake -S . -B "$BUILD_DIR" -DENABLE_SANITIZERS=OFF
  echo "Building project with IKOS…"
  echo "Y" | ikos-scan cmake --build "$BUILD_DIR"
  echo "Generating IKOS report…"
  ikos-view "$BUILD_DIR/ceeify.db"
}

clean() {
    rm -rf -- "$BUILD_DIR"
}

# If a function name was passed, call it
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    set -e

    if [[ $# -gt 0 ]]; then
        "$@"
    else
        echo "Usage: $0 <command>"
    fi
fi