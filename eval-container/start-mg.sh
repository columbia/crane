#!/bin/bash


CONTAINER="u1"
CONTAINER_IP="10.0.3.111"
KEY="$HOME/.ssh/lxc_priv_key"
EX_PROG_NAME="mg-server"
EX_CMD="/home/heming/hku/m-smr/apps/mongoose/mg-server -I /usr/bin/php-cgi -p 7000 -t 1 &"
if [ "$#" != "2" ]; then
	echo "Illegal number of parameters."
       echo "Usage: $0 <server program name> <command to run the server in container $CONTAINER>"
       echo "$0 $EX_PROG_NAME \"$EX_CMD\""
       exit 1;
fi
PROG_NAME=$1
CMD=$2
echo ""
echo "Starting the server $PROG_NAME in lxc $CONTAINER, with IP $USER@$CONTAINER_IP, \
	with command: \"$CMD\""

sudo lxc-stop -n $CONTAINER
sudo lxc-start -n $CONTAINER
sleep 5
ssh -i $KEY $USER@$CONTAINER_IP "tmux start-server; tmux new-session -d -s tmux_session"
ssh -i $KEY $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"$CMD\" C-m"
PID=`sudo lxc-attach -n $CONTAINER -- ps -e | grep $PROG_NAME | awk '{print $1}'`
echo "$PROG_NAME with pid ($PID) is started. If pid is empty, contact developers."
echo ""
echo "You can run the command to checkpoint the process."
echo "> ./checkpoint-restore.sh checkpoint $PROG_NAME ./checkpoint"
echo ""
echo "If checkpoint succeeds, you can run the command to restore the process."
echo "> ./checkpoint-restore.sh restore $PROG_NAME checkpoint-$PID.tar.gz"

