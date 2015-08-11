#!/bin/bash
CONTAINER="u1"
CONTAINER_IP="10.0.3.111"
CMD="/home/heming/hku/m-smr/apps/mongoose/mg-server -I /usr/bin/php-cgi -p 7000 -t 1 &"
PROG_NAME="mg-server"

sudo lxc-stop -n $CONTAINER
sudo lxc-start -n $CONTAINER
sleep 5
ssh $USER@$CONTAINER_IP "tmux start-server; tmux new-session -d -s tmux_session"
ssh $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"$CMD\" C-m"
PID=`sudo lxc-attach -n $CONTAINER -- ps -e | grep $PROG_NAME | awk '{print $1}'`
echo $PID
