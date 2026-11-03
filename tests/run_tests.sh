#!/bin/sh
# Run from the repo root:  sh tests/run_tests.sh
set -e
mkdir -p build/tests
cc -std=gnu11 -Wall -Wextra \
   -Itests/stubs -IInc/drivers/logic -IInc/drivers/wrappers \
   tests/test_main.c Src/logic/fsm.c Src/logic/angles.c \
   -lm -o build/tests/unit_tests
./build/tests/unit_tests
