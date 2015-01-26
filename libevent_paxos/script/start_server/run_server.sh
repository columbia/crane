#!/bin/bash

CUR_DIR=$(pwd)

SERVER_PROGRAM=${CUR_DIR}/../../target/server.out

CONFIG_PATH=${CUR_DIR}/../../target/nodes.cfg
SUFFIX=".log"

echo ${SERVER_PROGRAM}



export LD_LIBRARY_PATH=${CUR_DIR}/../../.local/lib

${SERVER_PROGRAM} -n 0 -m s -c ${CONFIG_PATH} >>node_start_0${SUFFIX}  2>>node_start_0_err${SUFFIX}  &
${SERVER_PROGRAM} -n 1 -m r -c ${CONFIG_PATH} >>node_start_1${SUFFIX} 2>>node_start_1_err${SUFFIX}  &
${SERVER_PROGRAM} -n 2 -m r -c ${CONFIG_PATH} >>node_start_2${SUFFIX}  2>>node_start_2_err${SUFFIX}  &

