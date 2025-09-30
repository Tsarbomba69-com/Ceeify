#!/usr/bin/env bash

# Generate build system files
gen() {
    cmake -S . -B ./build "$@"
}

# Build the project
b() {
    cmake --build ./build "$@"
}