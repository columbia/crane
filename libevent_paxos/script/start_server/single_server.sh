#!/bin/bash

CUR_DIR=$(pwd)

SERVER_PROGRAM=${CUR_DIR}/../../target/server.out

CONFIG_PATH=${CUR_DIR}/../../target/nodes.cfg
SUFFIX=".log"

echo ${SERVER_PROGRAM}



export LD_LIBRARY_PATH=${CUR_DIR}/../../.local/lib

${SERVER_PROGRAM} -n 0 -m s -c ${CONFIG_PATH} 1>node_start_0${SUFFIX} 2>node_start_0_err${SUFFIX}

