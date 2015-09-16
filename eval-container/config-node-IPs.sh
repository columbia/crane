#!/bin/bash

if [ "$#" != "3" ]; then
	echo "Illegal number of parameters."
       echo "Usage: $0 <primary IP> <backup1 IP> <backup2 IP>"
       exit 1;
fi

PRIMARY_IP=$1
BACKUP1_IP=$2
BACKUP2_IP=$3

# First, change the IPs in the eval-container directory.
cd $MSMR_ROOT/eval-container
CUR=`pwd`
echo "Rewriting $CUR/head..."
echo $PRIMARY_IP > head
echo "Rewriting $CUR/worker1..."
echo $BACKUP1_IP > worker1
echo "Rewriting $CUR/worker2..."
echo $BACKUP2_IP > worker2
echo "Rewriting $CUR/hostfile..."
cat worker1 > hostfile
cat worker2 >> hostfile
cat head >> hostfile

# Second, change the IPs in the nodes.current.options
cp $MSMR_ROOT/libevent_paxos/target/nodes.local.cfg nodes.local.cfg
echo "Updating $CUR/nodes.local.cfg... Will use this cfg file from now on."
CUR_PRIMARY_IP=`cat nodes.local.cfg | grep primary -m 1 | awk '{print $4}'`
CUR_BACKUP1_IP=`cat nodes.local.cfg | grep backup1 -m 1 | awk '{print $4}'`
CUR_BACKUP2_IP=`cat nodes.local.cfg | grep backup2 -m 1 | awk '{print $4}'`
echo "Raplace the IP set ($CUR_PRIMARY_IP $CUR_BACKUP1_IP $CUR_BACKUP2_IP) to new one ($PRIMARY_IP $BACKUP1_IP $BACKUP2_IP)"

sed -i -e "s/$CUR_PRIMARY_IP/$PRIMARY_IP/g" nodes.local.cfg
sed -i -e "s/$CUR_BACKUP1_IP/$BACKUP1_IP/g" nodes.local.cfg
sed -i -e "s/$CUR_BACKUP2_IP/$BACKUP2_IP/g" nodes.local.cfg

