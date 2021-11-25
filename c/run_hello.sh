#!/usr/bin/env bash

CFLAGS="-fsanitize=address -fsanitize-recover=address -fno-stack-protector -fno-omit-frame-pointer -fno-var-tracking -g1 -O1"

gcc $CFLAGS -Wall -o hello hello.c
gcc $CFLAGS -Wall -o simple.awk simple.awk.c

#export ASAN_OPTIONS="detect_leaks=1:halt_on_error=0:malloc_context_size=15:log_path=${HOME}/asan_test.log:suppressions=$SUPP_FILE:detect_stack_use_after_return=1:quarantine_size=4194304:handle_segv=1"
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=0:malloc_context_size=15:suppressions=$SUPP_FILE:detect_stack_use_after_return=1:quarantine_size=4194304:handle_segv=1"
./hello
./simple.awk "aaa[" simple.awk.c
