#!/bin/bash

set -e
set -x

wget http://www.iausofa.org/2015_0209_F/sofa_f-20150209_a.tar.gz -O /tmp/sofa.tgz
tar -xzf /tmp/sofa.tgz
cd sofa/20150209_a/f77/src/ && make && make test && cd ../../../../

mkdir build
cd build

wget ftp://ftp.astron.nl/outgoing/Measures/WSRT_Measures.ztar
tar zxvf WSRT_Measures.ztar


# We don't want to build/test Python3 support for OSX since that requires a rebuild of boost-python.
# This will take ages and will timeout the travis build.
#
if [ "$TRAVIS_OS_NAME" = osx ]; then
    cmake .. \
        -DUSE_FFTW3=ON \
        -DBUILD_TESTING=ON \
        -DUSE_OPENMP=OFF \
        -DUSE_HDF5=ON \
        -DBUILD_PYTHON=ON \
        -DBUILD_PYTHON3=OFF \
        -DDATA_DIR=$PWD \
        -DSOFA_ROOT_DIR=$HOME \
        -DCXX11=$CXX11 \
        -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/installed
else
    cmake .. \
        -DUSE_FFTW3=ON \
        -DBUILD_TESTING=ON \
        -DUSE_OPENMP=OFF \
        -DUSE_HDF5=ON \
        -DBUILD_PYTHON=ON \
        -DBUILD_PYTHON3=ON \
        -DDATA_DIR=$PWD \
        -DSOFA_ROOT_DIR=$HOME \
        -DCXX11=$CXX11 \
        -DCMAKE_INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/installed
fi



