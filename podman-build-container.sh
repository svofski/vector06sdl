#!/bin/bash

set -e
mkdir -p extrafiles && cd extrafiles
if ! test -e boost_1_79_0.tar.bz2 ; then
    wget https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.bz2
fi
cd ..
podman build -t v06x .
