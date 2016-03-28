#!/bin/bash

set -e

if [[ -z "$LLVM_INSTALL" ]]; then
    echo LLVM_INSTALL must be set
    exit 1
fi

if [[ -z "$LLVM_HOME" ]]; then
    echo LLVM_HOME must be set
    exit 1
fi

rm -rf $LLVM_INSTALL
rm -rf $LLVM_HOME

if [[ ! -d $OMP_TO_HCLIB_HOME ]]; then
    git clone git@github.com:agrippa/omp_to_hclib.git $OMP_TO_HCLIB_HOME
fi

LLVM_HOME_DIR=$(dirname $LLVM_HOME)

if [[ -z "$OMP_TO_HCLIB_LLVM_VERSION" ]]; then
    OMP_TO_HCLIB_LLVM_VERSION=3.8.0
fi

LLVM_VERSION=$OMP_TO_HCLIB_LLVM_VERSION
CLANG_VERSION=$OMP_TO_HCLIB_LLVM_VERSION
COMPILER_RT_VERSION=$OMP_TO_HCLIB_LLVM_VERSION

LLVM_TAR=llvm-${LLVM_VERSION}.src
CLANG_TAR=cfe-${CLANG_VERSION}.src
COMPILER_RT_TAR=compiler-rt-${COMPILER_RT_VERSION}.src

# Download source
cd $LLVM_HOME_DIR && rm -f $LLVM_TAR.tar.xz && \
       wget http://llvm.org/releases/${LLVM_VERSION}/$LLVM_TAR.tar.xz
cd $LLVM_HOME_DIR && rm -f $CLANG_TAR.tar.xz && \
       wget http://llvm.org/releases/${CLANG_VERSION}/$CLANG_TAR.tar.xz
cd $LLVM_HOME_DIR && rm -f $COMPILER_RT_TAR.tar.xz && \
       wget http://llvm.org/releases/${COMPILER_RT_VERSION}/$COMPILER_RT_TAR.tar.xz

# Build directory structure for llvm, clang, compiler-rt
cd $LLVM_HOME_DIR && tar xf $LLVM_TAR.tar.xz && mv $LLVM_TAR $LLVM_HOME
cd $LLVM_HOME_DIR && tar xf $CLANG_TAR.tar.xz && \
       mv $CLANG_TAR $LLVM_HOME/tools/clang
cd $LLVM_HOME_DIR && tar xf $COMPILER_RT_TAR.tar.xz && \
       mv $COMPILER_RT_TAR $LLVM_HOME/projects/compiler-rt

# Create installation directory
mkdir -p $LLVM_INSTALL
cd $LLVM_INSTALL && CC=gcc CXX=g++ $LLVM_HOME/configure --enable-optimized=no --enable-profiling=no
cd $LLVM_INSTALL && make -j 6
