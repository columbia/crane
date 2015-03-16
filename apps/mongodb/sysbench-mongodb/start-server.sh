#!/bin/bash

# simple script to run against running MongoDB/TokuMX server localhost:(default port)

# enable passing different config files

if [ ! $1 ];
then
    echo "Usage: ./start-server <server port>"
    exit 1;
fi


# Heming cleanup work.
PORT=7000

cd $MSMR_ROOT/apps/mongodb/sysbench-mongodb
#rm -rf db-dir
mkdir -p db-dir
killall -9 mongo mongod

echo "Starting mongodb server, please wait for about 15 seconds..."
#L2D_PRELOAD=$XTERN_ROOT/dync_hook/interpose.so \
$MSMR_ROOT/apps/mongodb/install/bin/mongod --port $PORT --dbpath=$PWD/db-dir --quiet &> mongodb.log &
sleep 15;

