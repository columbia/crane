#!/bin/bash            

CONTAINER="u1"
CONTAINER_IP="10.0.3.111"
CMD="sudo criu restore -d -D /home/heming/./checkpoint  --shell-job --tcp-established --file-locks"
PROG_NAME="mg-server"

sudo lxc-stop -n $CONTAINER
sudo lxc-start -n $CONTAINER
sleep 5

ssh $USER@$CONTAINER_IP "tmux start-server; tmux new-session -d -s tmux_session"
#ssh -t $USER@$CONTAINER_IP "tmux attach-session -t tmux_session"
ssh $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"$CMD &> out.txt\" C-m"
sudo lxc-attach -n $CONTAINER -- cat $HOME/out.txt
sleep 1

ssh $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"$CMD &> out.txt\" C-m"
sudo lxc-attach -n $CONTAINER -- cat $HOME/out.txt
sleep 1

#ssh $USER@$CONTAINER_IP "tmux send-keys -t tmux_session \"$CMD &> restore.txt\" C-m"
#sudo lxc-attach -n $CONTAINER -- cat $HOME/restore.txt

#parallel-ssh -l $USER -v -p 1 -x "-oStrictHostKeyChecking=no  -i $HOME/.ssh/lxc_priv_key" 
#	-i -t 10 -h $MSMR_ROOT/eval-container/u1 "$CMD"

#ssh -t $USER@$CONTAINER_IP "tmux start-server; tmux new-session -d -s tmux_session"
#ssh -t $USER@$CONTAINER_IP "tmux send-keys -t tmux_session:0 \"$CMD\" C-m"
#ssh -t $USER@$CONTAINER_IP "tmux select-window -t tmux_session:runstuff"
#ssh -t $USER@$CONTAINER_IP "tmux attach-session -t tmux_session"
