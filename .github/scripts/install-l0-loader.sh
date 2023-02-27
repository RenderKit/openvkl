#!/bin/bash -e
unset LEVEL_ZERO_ROOT
pushd `pwd`
cd /tmp
wget https://github.com/oneapi-src/level-zero/releases/download/v1.9.4/level-zero-devel_1.9.4+u18.04_amd64.deb
wget https://github.com/oneapi-src/level-zero/releases/download/v1.9.4/level-zero_1.9.4+u18.04_amd64.deb
dpkg -i ./level-zero*.deb
rm ./level-zero*.deb
popd