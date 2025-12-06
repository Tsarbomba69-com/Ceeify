#!/usr/bin/env bash

set -e

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

# If a function name was passed, call it
if [[ $# -gt 0 ]]; then
  "$@"
else
  echo "Usage: $0 <command> [args...]"
  echo "Available commands: gen, build, valgrind_run, valgrind_test, lint"
fi