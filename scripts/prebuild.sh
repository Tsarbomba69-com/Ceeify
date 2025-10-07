#!/bin/bash

export ASAN_OPTIONS="detect_leaks=1:detect_stack_use_after_return=1:abort_on_error=1:halt_on_error=1"
export LSAN_OPTIONS="verbosity=1:log_threads=1"
export UBSAN_OPTIONS="abort_on_error=1:halt_on_error=1"
