#!/bin/bash

if [ "$#" != "3" ]; then
	echo "Illegal number of parameters."
       echo "Usage: $0 checkpoint|restore program_name <checkpoint or restore directory>"
       echo "$0 checkpoint mg-server ./checkpoint"
       echo "$0 restore mg-server ./checkpoint"
       exit 1;
fi

OP=$1
PROG_NAME=$2
DIR=$3
CONTAINTER="u1"

echo "Running command: $0 $OP $PROG_NAME $DIR"

if [ "$OP" != "checkpoint" ]; then

# First, checkpoint the server application within the container.
	ssh ubuntu@$(sudo lxc-info -i -H -n $CONTAINTER) 
	# TBD: what if multiple processes?
	PID=`ps -e | grep $PROG_NAME | awk '{print $1}'`
	echo "Checkpointing process with pid $PID into directory $DIR ..."
	rm -rf $DIR
	mkdir $DIR
	sudo criu dump -D $DIR -t $PID --shell-job --tcp-established --file-locks
	exit

# Second, checkpoint the file system of the container, and bdb storage of the proxy process.
      sudo lxc-stop -n $CONTAINTER
      sudo su root
      cd /var/lib/lxc/$CONTAINTER/snaps
      CUR_DIR=`pwd`
      echo "Diffing $CUR_DIR/base/rootfs/$HOME and $CUR_DIR/../rootfs/$HOME"
      diff -ruN $CUR_DIR/base/rootfs/$HOME $CUR_DIR/../rootfs/$HOME &> filesystem-checkpoint.patch
      chmod 777 filesystem-checkpoint.patch
      echo "Compressing process checkpoint and file system of the server, and bdb storage of the proxy at $HOME/.db into $HOME/checkpoint-$PID.tar.gz..."
      tar zcvf checkpoint-$PID.tar.gz filesystem-checkpoint.path $HOME/.db
      exit
      sudo mv $CUR_DIR/checkpoint-$PID.tar.gz $HOME/
fi

if [ "$OP" != "restore" ]; then
# First, checkpoint the file system of the container.

# Second, restore the server application within the container, and bdb storage of the proxy process.
sudo criu restore -d -D $DIR                         --shell-job --tcp-established --file-locks

fi

