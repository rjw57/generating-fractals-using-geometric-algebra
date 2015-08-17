#!/bin/sh

g++ -o cgatest `pkg-config libcga --cflags --libs` cgatest.cc || exit 1
./cgatest bob.ppm
