#!/bin/bash

process_host() {
ssh $USER@$HOST.cs.columbia.edu <<'ENDSSH'
#commands to run on remote host
cd /dev/shm/dmt_out/
HOST=`hostname`
sudo rm ./$HOST-*.log
for f in *.log
do
  NUM=`grep -c accept $f`
  if [ "$NUM"X != "0X" ]; then
        echo "$f is a server schedule file on $HOST machine."
        sudo cp ./$f ./$HOST-$f
  fi
done
ENDSSH

scp $HOST.cs.columbia.edu:/dev/shm/dmt_out/$HOST* scheds/
}

# Main.
rm -rf scheds
mkdir scheds

HOST="bug03"
process_host
LINE=0
cd scheds
for f in $HOST-*.log
do
	LINE=`grep -n close $HOST-* | tail -1 | awk -F ":" '{print $1}'`
	echo "Last close() in $HOST schedule file is $LINE"
done
cd ..

HOST="bug01"
process_host

HOST="bug02"
process_host

cd scheds
for f in *.log
do
	echo "Truncating $f after line $LINE (the last close() operation)..."
	sed -i "$LINE,$ d" $f
	echo "number of lines: $(wc -l < $f)"
done

cd ..

