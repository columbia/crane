#!/bin/bash

# heming@heming:~/hku/m-smr/eval-container$
# while [ 1 ]; do (  sleep 1; ./utility-scripts/parallel-curl.sh 7000) done 
# while [ 1 ]; do (  sleep 1;  LD_PRELOAD=$MSMR_ROOT/libevent_paxos/client-ld-preload/libclilib.so ~/hku/m-smr/eval-container/utility-scripts/parallel-curl.sh 9000) done

if [ ! $1 ];
then
	echo "./$0 7000"
	exit 1;
fi


PORT=$1
IP="127.0.0.1"

rm /home/heming/hku/m-smr/apps/apache/install/htdocs/result.php &> /dev/null
echo "" > result.txt
sync

curl --max-time 2 -i -T test.txt http://$IP:$PORT/ &
P1=$!

#sleep 0.001

curl --max-time 2 -o result.txt http://$IP:$PORT/result.php &
P2=$!





sleep 3

#kill -9 $P1 &> /dev/null
#kill -9 $P2 &> /dev/null
echo "=================="
cat result.txt
echo "=================="
exit 0
