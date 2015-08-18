#!/bin/bash

process_host() {
ssh $USER@$HOST.cs.columbia.edu <<'ENDSSH'
#commands to run on remote host
cd /dev/shm/dmt_out/
HOST=`hostname`
sudo rm ./$HOST*.log
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

RESULT=0
STD_FILE=""
HOST="bug03"
echo "Collecting schedules from /dev/shm/dmt_out/ directory on bug0* machines..."
process_host &> /dev/null
LINE=0
cd scheds
for f in $HOST-*.log
do
	LINE=`grep -n close $HOST-* | tail -1 | awk -F ":" '{print $1}'`
	echo "Last close() in the schedule file $f from $HOST machine is $LINE"
	STD_FILE=$f
done
cd ..

HOST="bug01"
process_host &> /dev/null

HOST="bug02"
process_host &> /dev/null

(( LINE++ ))
cd scheds
for f in *.log
do
	echo "Truncating $f after line $LINE (the last close() operation)..."
	sed -i "$LINE,$ d" $f
done

mkdir results
for f in *.log
do
  if [ $f != $STD_FILE ]; then
    DIFF="diff-$STD_FILE-$f"
    diff -ruN $STD_FILE $f > results/$DIFF
    CNT=`wc -c results/$DIFF | awk '{print $1}'`
    echo "Diff $STD_FILE and $f, lines of difference is $CNT."
    echo ""
    if [ $CNT != 0 ]; then
      echo "$STD_FILE and $f are not the same, see diff result in scheds/results/$DIFF"
      RESULT=1
    fi
  fi
done

cd ..

if [ $RESULT != 0 ]; then
  echo "Some schedules are not the same, please see ./scheds/results/"
else
  echo ""
  echo "All schedules are the same. Great!"
fi

#echo ""
#echo ""
#echo "Please truncate all files in the scheds directory with this line number: $LINE"
#echo "Command to do so: cd scheds; ls . | xargs sed -i '$LINE,$ d'"

