#!/usr/bin/env bash

# Generate build system files
gen() {
    cmake -S . -B ./build "$@"
}

# Build the project
build() {
    cmake --build ./build "$@"
}

valgrind_all() {
  valgrind --leak-check=full --show-leak-kinds=all ./build/ceeify
}

valgrind_test() {
  valgrind --leak-check=full --show-leak-kinds=all ./build/test_ceeify
}

lint() {
  dir="$1"
  if [ -z "$dir" ]; then
    echo "Usage: f <directory>"
    return 1
  fi

  find "$dir" \( -name "*.c" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
}