#!/bin/bash

function finish {
    libsboxd stop
    wait
}
trap finish EXIT

libsboxd start &
sleep 0.5
time ./client 10 || exit 1

