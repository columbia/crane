#!/bin/bash

#sudo lxc-stop -n u1
#sudo lxc-start -n u1
#sleep 1
#ssh -t $USER@10.0.3.111 "tmux start-server; tmux new-session -d -s tmux_session"
#ssh -t $USER@10.0.3.111 "tmux send-keys -t tmux_session \"$MSMR_ROOT/apps/mongoose/mg-server -I /usr/bin/php-cgi -p 7000 -t 1 &\" C-m"

# checkpoint.
./checkpoint-restore.sh checkpoint mg-server ./checkpoint
