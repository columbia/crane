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
CONTAINER="u1"
CRIU_ARGS=" --shell-job --tcp-established --file-locks"
SNAPS_DIR="/var/lib/lxc/$CONTAINER/snaps"

echo "Running command: $0 $OP $PROG_NAME $DIR"

if [ "$OP" == "checkpoint" ]; then

# First, checkpoint the server application within the container.
	# TBD: what if multiple processes?
	PID=`sudo lxc-attach -n $CONTAINER -- ps -e | grep $PROG_NAME | awk '{print $1}'`
	echo "Checkpointing process with pid $PID into directory $DIR ..."
	sudo lxc-attach -n $CONTAINER -- rm -rf $HOME/$DIR
	sudo lxc-attach -n $CONTAINER -- mkdir $HOME/$DIR
	sudo lxc-attach -n $CONTAINER -- sudo criu dump -D $HOME/$DIR -t $PID $CRIU_ARGS
	#exit 0

# Second, checkpoint the file system of the container, and bdb storage of the proxy process.
      sudo lxc-stop -n $CONTAINER
      echo "Diffing $SNAPS_DIR/base/rootfs/$HOME and $SNAPS_DIR/../rootfs/$HOME"
      sudo diff -ruN --text --exclude=.bash_history $SNAPS_DIR/base/rootfs/$HOME $SNAPS_DIR/../rootfs/$HOME &> filesystem-checkpoint.patch
      echo "Compressing process checkpoint and file system of the server, and bdb storage of the proxy at $HOME/.db into $HOME/checkpoint-$PID.tar.gz..."
      cp -r $HOME/.db db
      sudo tar zcvf checkpoint-$PID.tar.gz filesystem-checkpoint.patch db
      rm -rf db

# Resume process and the container.
	sudo lxc-start -n $CONTAINER
	sleep 1
	ssh -t $USER@$(sudo lxc-info -i -H -n $CONTAINER) "tmux start-server; tmux new-session -d -s tmux_session"
	ssh -t $USER@$(sudo lxc-info -i -H -n $CONTAINER) "tmux send-keys -t tmux_session \"sudo criu restore -d -D $HOME/$DIR $CRIU_ARGS\" C-m"
	sudo lxc-attach -n $CONTAINER -- sudo criu restore -d -D $HOME/$DIR $CRIU_ARGS
	PID=`sudo lxc-attach -n $CONTAINER -- ps -e | grep $PROG_NAME | awk '{print $1}'`
	echo "Process pid $PID has been successfully checkpointed with its file system at checkpoint-$PID.tar.gz in current directory."
fi

#if [ "$OP" != "restore" ]; then
# First, checkpoint the file system of the container.

# Second, restore the server application within the container, and bdb storage of the proxy process.
#sudo criu restore -d -D $DIR                         $CRIU_ARGS

#fi

