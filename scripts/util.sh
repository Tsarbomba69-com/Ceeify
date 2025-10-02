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
  find . \( -name "*.c" -o -name "*.cpp" \) -print0 | xargs -0 -n1 gcc -o /tmp/a.out && valgrind --leak-check=full /tmp/a.out
}

lint() {
  dir="$1"
  if [ -z "$dir" ]; then
    echo "Usage: f <directory>"
    return 1
  fi

  find "$dir" \( -name "*.c" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
}