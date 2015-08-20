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
  NUM=`grep -c Function $f`
  if [ "$NUM"X != "0X" ]; then
        echo "$f is a server network output file on $HOST machine."
        sudo cp ./$f ./$HOST-$f
  fi
done
ENDSSH

scp $HOST.cs.columbia.edu:/dev/shm/dmt_out/$HOST* scheds/
sync
}

# Main.
rm -rf scheds
mkdir scheds

RESULT=0
STD_FILE=""
STD_OUTPUT_FILE=""
HOST="bug03"
echo "Collecting schedules from /dev/shm/dmt_out/ directory on bug0* machines..."
process_host &> /dev/null
LINE=0
cd scheds
# find leader output.
for f in $HOST-network*.log
do
	LINE=`grep -n Function $f`
	if [ "$LINE"X != "X" ]; then 
		echo "File $f contains network output on host $HOST. Use it as standard output file."
		STD_OUTPUT_FILE=$f
	fi
done
#find leader schedule.
for f in $HOST-serializer*.log
do
	LINE=`grep -n close $f | tail -1 | awk -F ":" '{print $1}'`
	echo "Last close() in the schedule file $f from $HOST machine is $LINE. Use it as standard schedule file."
	STD_FILE=$f
done
cd ..

HOST="bug01"
process_host &> /dev/null

HOST="bug02"
process_host &> /dev/null

(( LINE++ ))
cd scheds
for f in *serializer*.log
do
	echo "Truncating $f after line $LINE (the last close() operation)..."
	sed -i "$LINE,$ d" $f
done

mkdir results
# diff schedules.
for f in *serializer*.log
do
  if [ $f != $STD_FILE ]; then
    DIFF="diff-$STD_FILE-$f"
    #cat $STD_FILE | grep -v "RRScheduler::wait" | awk '{print $1" "$3" "$4" "$5}' > start.txt
    #cat $f | grep -v "RRScheduler::wait" | awk '{print $1" "$3" "$4" "$5}' > end.txt
    #diff -ruN start.txt end.txt > results/$DIFF
    diff -ruN $STD_FILE $f > results/$DIFF
    CNT=`wc -c results/$DIFF | awk '{print $1}'`
    echo "Diff $STD_FILE and $f, lines of difference is $CNT."
    echo ""
    if [ $CNT"X" != "0X" ]; then
      #echo "$STD_FILE and $f are not the same, see diff result in scheds/results/$DIFF"
      RESULT=1
    fi
  fi
done

if [ $RESULT != 0 ]; then
  echo ""
  #echo "Some schedules are not the same, please see ./scheds/results/"
else
  echo ""
  #echo "All schedules are the same. Great!"
fi

# diff outputs.
RESULT=0
for f in *output*.log
do
  if [ $f != $STD_OUTPUT_FILE ]; then
    DIFF="diff-$STD_OUTPUT_FILE-$f"
    diff -ruN $STD_OUTPUT_FILE $f > results/$DIFF
    CNT=`wc -c results/$DIFF | awk '{print $1}'`
    echo "Diff $STD_OUTPUT_FILE and $f, lines of difference is $CNT."
    echo ""
    if [ $CNT"X" != "0X" ]; then
      echo "$STD_OUTPUT_FILE and $f are not the same, see diff result in scheds/results/$DIFF"
      RESULT=1
    fi
  fi
done


if [ $RESULT"X" != "0X" ]; then
  echo ""
  echo "Some network outputs are not the same, please see ./scheds/results/"
else
  echo ""
  echo "All network outputs are the same. Great!"
fi

cd ..


