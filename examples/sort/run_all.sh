#!/bin/bash
for test in tests/0??; do
    ./invoke ${test} $1
done
