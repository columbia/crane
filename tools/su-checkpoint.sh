#!/bin/bash

if [ ! $1 ];
then
        echo "Usage: $0 <name of the process, must be the only one process running with this name>"
        echo "$0 mg-server"
        exit 1;
fi

NAME=$1;
PID=`pgrep $NAME`;
DIR=$PWD/$NAME-$PID-checkpoint
echo "Dummping process checkpoint of $1 (pid $PID) to directory $DIR";
rm -rf $DIR
mkdir $DIR
CRIU_PATH=/home/bluecloudmatrix/criu/deps/criu-x86_64
CRIU_PROGRAM=$CRIU_PATH/criu
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CRIU_PATH/../x86_64-linux-gnu/lib

# Dump.
$CRIU_PROGRAM dump -D $DIR -t $PID --shell-job --tcp-established --leave-running --file-locks
