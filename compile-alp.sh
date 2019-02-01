#!/bin/bash

# Working setup to cross-compile Windows binaries for Qt Coin hosted on a
# Ubuntu 16.04 box using non-Canonical ppas for MXE and Qt5.7:
# deb http://pkg.mxe.cc/repos/apt/debian wheezy main

# Doesn't seem to pass the QT directives through, though.

# Basic path bindings

PATH=/home/username/Shared/ALP/mxe/usr/bin:$PATH
MXE_PATH=/home/username/Shared/ALP/mxe
MXE_INCLUDE_PATH=/home/username/Shared/ALP/mxe/usr/i686-w64-mingw32.static/include
MXE_LIB_PATH=/home/username/Shared/ALP/mxe/usr/i686-w64-mingw32.static/lib

# Belt and braces

CXXFLAGS="-std=gnu++11 -march=i686"
LDFLAGS="-march=i686"
target="i686-w64-mingw32.static"

# Particulars for cross-compiling

export BOOST_LIB_SUFFIX=-mt
export BOOST_THREAD_LIB_SUFFIX=_win32-mt
export BOOST_INCLUDE_PATH=${MXE_INCLUDE_PATH}/boost
export BOOST_LIB_PATH=${MXE_LIB_PATH}
export OPENSSL_INCLUDE_PATH=${MXE_INCLUDE_PATH}/openssl
export OPENSSL_LIB_PATH=${MXE_LIB_PATH}
export BDB_INCLUDE_PATH=${MXE_INCLUDE_PATH}
export BDB_LIB_PATH=${MXE_LIB_PATH}
export MINIUPNPC_INCLUDE_PATH=${MXE_INCLUDE_PATH}
export MINIUPNPC_LIB_PATH=${MXE_INCLUDE_LIB}
export MINIUPNP_STATICLIB=${MXE_INCLUDE_LIB}
export QMAKE_LRELEASE=${MXE_PATH}/usr/${target}/qt5/bin/lrelease
export PATH=/home/vova/Shared/ALP/mxe/usr/bin:$PATH

# Call qmake to create Makefile.[Release|Debug]

${target}-qmake-qt5 \
    MXE=1 \
    USE_O3=1 \
    ENABLE_WALLET=1 \
    USE_QRCODE=0 \
    USE_UPNPC=1 \
    FIRST_CLASS_MESSAGING=1 \
    RELEASE=1 \
    BOOST_LIB_SUFFIX=${BOOST_LIB_SUFFIX} \
    BOOST_THREAD_LIB_SUFFIX=${BOOST_THREAD_LIB_SUFFIX} \
    BOOST_INCLUDE_PATH=${BOOST_INCLUDE_PATH} \
    BOOST_LIB_PATH=${BOOST_LIB_PATH} \
    OPENSSL_INCLUDE_PATH=${OPENSSL_INCLUDE_PATH} \
    OPENSSL_LIB_PATH=${OPENSSL_LIB_PATH} \
    BDB_INCLUDE_PATH=${BDB_INCLUDE_PATH} \
    BDB_LIB_PATH=${BDB_LIB_PATH} \
    MINIUPNPC_INCLUDE_PATH=${MINIUPNPC_INCLUDE_PATH} \
    MINIUPNPC_LIB_PATH=${MINIUPNPC_LIB_PATH} \
    MINIUPNP_STATICLIB=${MINIUPNP_STATICLIB} \
    QMAKE_LRELEASE=${QMAKE_LRELEASE} alphacon-qt.pro 

# Go for it. If successful, Windows binary will be written out to ./release/COin-name-qt.exe
cd src/leveldb
TARGET_OS=NATIVE_WINDOWS make libleveldb.a libmemenv.a CC=/home/vova/Shared/ALP/mxe/usr/bin/i686-w64-mingw32.static-gcc CXX=/home/vova/Shared/ALP/mxe/usr/bin/i686-w64-mingw32.static-g++

cd ../..

make -f Makefile.Release CXXFLAGS="-DQT_GUI -DQT_NO_PRINTER -std=gnu++11 -march=native" LDFLAGS="-march=native" -j4
