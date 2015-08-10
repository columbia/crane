#!/bin/bash

if [ "$#" != "3" ]; then
	echo "Illegal number of parameters."
       echo "Usage: $0 checkpoint|restore program_name <checkpoint or restore directory>"
       echo "$0 checkpoint mg-server ./checkpoint"
       echo "$0 restore mg-server checkpoint.tar.gz"
       exit 1;
fi

OP=$1
PROG_NAME=$2
DIR=$3
CONTAINER="u1"
CRIU_ARGS=" --shell-job --tcp-established --file-locks"
SNAPS_DIR="/var/lib/lxc/$CONTAINER/snaps"
FS_BASE_DIR="$SNAPS_DIR/base"
EXCLUDES=".bash_history"
#	$MSMR_ROOT/xtern 
#	$MSMR_ROOT/libevent_paxos 
#	$MSMR_ROOT/tools 
#	$MSMR_ROOT/eval 
#	$MSMR_ROOT/eval-container 
#	$MSMR_ROOT/eval-multiMachine
CONTAINER_IP="10.0.3.111"

echo "Running command: $0 $OP $PROG_NAME $DIR"

if [ "$OP" == "checkpoint" ]; then

# First, checkpoint the server application within the container.
	# TBD: what if multiple processes?
	PID=`sudo lxc-attach -n $CONTAINER -- ps -e | grep $PROG_NAME | awk '{print $1}'`
	echo "Checkpointing process with pid $PID into directory $DIR ..."
	sudo lxc-attach -n $CONTAINER -- sudo rm -rf $HOME/$DIR
	sudo lxc-attach -n $CONTAINER -- mkdir $HOME/$DIR
	ssh -t $USER@$CONTAINER_IP "tmux start-server; tmux new-session -d -s tmux_session"
	ssh -t $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"sudo criu dump -t $PID -D $HOME/$DIR $CRIU_ARGS\" C-m"
	#sudo lxc-attach -n $CONTAINER -- sudo criu dump -D $HOME/$DIR -t $PID $CRIU_ARGS
	#exit 0

# Second, checkpoint the file system of the container, and bdb storage of the proxy process.
      sudo lxc-stop -n $CONTAINER
      echo "Diffing base file system state: $FS_BASE_DIR/rootfs/$HOME, and current file system state: $SNAPS_DIR/../rootfs/$HOME"
      sudo rm -rf start end
      sudo ln -s $FS_BASE_DIR/rootfs/$HOME start
      sudo ln -s $SNAPS_DIR/../rootfs/$HOME end
      sudo diff -ruN --text --exclude=$EXCLUDES start end &> filesystem-checkpoint.patch
      echo "Compressing process checkpoint and file system of the server, and bdb storage of the proxy at $HOME/.db into $HOME/checkpoint-$PID.tar.gz..."
      cp -r $HOME/.db db
      sudo tar zcvf checkpoint-$PID.tar.gz filesystem-checkpoint.patch db
      sudo rm -rf db start end

# Resume process and the container.
	sudo lxc-start -n $CONTAINER
	sleep 10
	ssh -t $USER@$CONTAINER_IP "tmux start-server; tmux new-session -d -s tmux_session"
	ssh -t $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"sudo criu restore -d -D $HOME/$DIR $CRIU_ARGS\" C-m"
	#sudo lxc-attach -n $CONTAINER -- sudo criu restore -d -D $HOME/$DIR $CRIU_ARGS
	PID=`sudo lxc-attach -n $CONTAINER -- ps -e | grep $PROG_NAME | awk '{print $1}'`
	echo "Process pid $PID has been successfully checkpointed with its file system at checkpoint-$PID.tar.gz in current directory."
       echo "To make sure checkpoint of the process succeeded, run this command below. It should contain a few *.img files."
       echo "> sudo lxc-attach -n $CONTAINER -- ls -l $HOME/$DIR"
fi

if [ "$OP" == "restore" ]; then
      tar zxvf $DIR
      echo "Restoring .db directory to $HOME/.db in the host OS..."
      rm $HOME/.db -rf
      mv db $HOME/.db
      echo "Restoring $HOME directory file system in the lxc container..."
      cp filesystem-checkpoint.patch $HOME
      sudo chmod 666 $HOME/filesystem-checkpoint.patch

# Second, restore the file system of the container.
      sudo lxc-stop -n $CONTAINER
      sleep 1
      echo "Restoring base file system in $FS_BASE_DIR, may take a few minutes..."
      sudo lxc-snapshot -n $CONTAINER -r base
      echo "Restoring current file system to the checkpoint moment, may take a few seconds..."
      sudo patch -d $SNAPS_DIR/../rootfs/$HOME -p1 < $HOME/filesystem-checkpoint.patch

# Resume process and the container.
	sudo lxc-start -n $CONTAINER
	sleep 10
	ssh -t $USER@$CONTAINER_IP "tmux start-server; tmux new-session -d -s tmux_session"
	ssh -t $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"sudo criu restore -d -D $HOME/checkpoint $CRIU_ARGS\" C-m"
	PID=`sudo lxc-attach -n $CONTAINER -- ps -e | grep $PROG_NAME | awk '{print $1}'`
	echo "Process pid $PID has been successfully checkpointed with its file system at checkpoint-$PID.tar.gz in current directory."
fi

