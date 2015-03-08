#! /bin/bash

CRIU_PATH=/home/liuqiushan/BCMatrix/criu/deps/criu-x86_64
CRIU_PROGRAM=${CRIU_PATH}/criu
# storing the checkpointed image files(a custom directory)
CRIU_IMAGE_PATH=${CRIU_PATH}/checkpoint

# checking whether the checkpointed progress is running or not
while true; do

# server.out is the progress of libevent_paxos
LIBEVENT_PAXOS_PID=$(pgrep "server.out")

echo "libevent_paxos pid:"
echo ${LIBEVENT_PAXOS_PID}

ps -fe|grep server.out |grep -v grep
if [ $? -eq 0 ];then

echo "catch it!"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CRIU_PATH}/../x86_64-linux-gnu/lib

${CRIU_PROGRAM} dump -D ${CRIU_IMAGE_PATH} -t ${LIBEVENT_PAXOS_PID} \
	--shell-job --tcp-established

${CRIU_PROGRAM} restore -d -D ${CRIU_IMAGE_PATH} --shell-job --tcp-established

fi

echo "sleeping 10 sec..."
sleep 10

done
