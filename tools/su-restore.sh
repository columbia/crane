#!/bin/bash

if [ ! $1 ];
then
        echo "Usage: $0 <absolute directory path of the checkpoint>"
        echo "$0 $PWD/mg-server-1234-checkpoint"
        exit 1;
fi

DIR=$1;
echo "Restoring process checkpoint from directory $DIR";
CRIU_PATH=/home/bluecloudmatrix/criu/deps/criu-x86_64
CRIU_PROGRAM=$CRIU_PATH/criu
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CRIU_PATH/../x86_64-linux-gnu/lib

# Restore
$CRIU_PROGRAM restore -d -D $DIR --shell-job --tcp-established --file-locks
