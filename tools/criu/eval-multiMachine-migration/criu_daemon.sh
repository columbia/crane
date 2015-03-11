#! /bin/bash

sleep 10

# the script should be run on bluecloudmatrix@bug02(a.k.a, Node 1)
# BTW, Leader node is bluecloudmatrix@bug01; Node 2 is bug03
CRIU_PATH=/home/bluecloudmatrix/criu/deps/criu-x86_64
CRIU_PROGRAM=${CRIU_PATH}/criu
# used for storing the image files of libevent_paxos process
CRIU_IMAGE_PATH=${CRIU_PATH}/checkpoint
# used for storing the image files of real server process, such as mongoose
CRIU_SERVER_IMGAE_PATH=${CRIU_PATH}/checkpoint_server

# checking whether the two processes which should be checkpointed are running
#while true; do

LIBEVENT_PAXOS_PID=$(pgrep "server.out")
echo "libevent_paxos pid:"
echo ${LIBEVENT_PAXOS_PID}

ps -fe|grep server.out |grep -v grep
if [ $? -eq 0 ];then

echo "catch it!"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CRIU_PATH}/../x86_64-linux-gnu/lib

# CRIU DUMP
${CRIU_PROGRAM} dump -D ${CRIU_IMAGE_PATH} -t ${LIBEVENT_PAXOS_PID} \
	--shell-job --tcp-established

sleep 5

# CRIU RESTORE
${CRIU_PROGRAM} restore -d -D ${CRIU_IMAGE_PATH} --shell-job --tcp-established

fi

#echo "slee 10..."
#sleep 10

#done
