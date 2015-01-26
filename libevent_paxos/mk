#! /bin/bash
#build dep for the program
# to be added
# build dep
# sudo apt-get install libtool

CUR_DIR=$(pwd)
LIB_PREFIX=${CUR_DIR}/.local/

mkdir -p ${LIB_PREFIX}

LIBCONFIG_VER=1.4.9
LIBCONFIG_NAME=libconfig-${LIBCONFIG_VER}

LIBEVENT_VER=2.0.21
LIBEVENT_NAME=libevent-release-${LIBEVENT_VER}-stable

BDB_VER=5.1.29
BDB_NAME=db-${BDB_VER}

if [ ! -d "dep-lib" ];then
    mkdir dep-lib
fi
cd dep-lib

if [ ! -f "${LIBCONFIG_NAME}.tar.gz" ];then
    wget http://www.hyperrealm.com/libconfig/${LIBCONFIG_NAME}.tar.gz
fi

if [ ! -d "${LIBCONFIG_NAME}" ];then
    tar -xvf ${LIBCONFIG_NAME}.tar.gz
fi

cd ${LIBCONFIG_NAME}
pwd
./configure --prefix=${LIB_PREFIX}
make;
make install;
cd ..


if [ ! -f "${LIBEVENT_NAME}.tar.gz" ];then
    wget --no-check-certificate https://github.com/downloads/libevent/libevent/${LIBEVENT_NAME}.tar.gz
    wget  https://codeload.github.com/libevent/libevent/tar.gz/release-${LIBEVENT_VER}-stable -O ${LIBEVENT_NAME}.tar.gz
fi

if [ ! -d "${LIBEVENT_NAME}" ];then
    tar -xvf ${LIBEVENT_NAME}.tar.gz
fi

cd ${LIBEVENT_NAME}
pwd
./autogen.sh
./configure --prefix=${LIB_PREFIX}
make;
make install;
cd ..


if [ ! -f "${BDB_NAME}.tar.gz" ];then
    wget http://download.oracle.com/berkeley-db/${BDB_NAME}.tar.gz
fi

if [ ! -d "${BDB_NAME}" ];then
    tar -xvf ${BDB_NAME}.tar.gz
fi

cd ${BDB_NAME}
pwd
cd build_unix
../dist/configure --prefix=${LIB_PREFIX}
make;
make install;
cd ..
cd ..

cd ..
