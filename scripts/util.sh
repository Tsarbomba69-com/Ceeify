#!/usr/bin/env bash

# Generate build system files
gen() {
    cmake -S . -B ./build "$@"
}

# Build the project 
build() {
    cmake --build ./build "$@"
}

valgrind_run() {
  valgrind --leak-check=full --show-leak-kinds=all --show-reachable=yes --track-origins=yes --num-callers=20 --track-fds=yes ./build/ceeify "$@"
}

valgrind_test() {
  valgrind --leak-check=full --show-leak-kinds=all --show-reachable=yes --track-origins=yes --num-callers=20 --track-fds=yes ./build/test_ceeify
}

lint() {
  dir="$1"
  if [ -z "$dir" ]; then
    echo "Usage: f <directory>"
    return 1
  fi

  find "$dir" \( -name "*.c" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
}