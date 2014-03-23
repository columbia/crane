#! /bin/sh

concoord object -o server_test.serverTest -p 1
sleep 3
concoord replica -o server_test.serverTest -a 127.0.0.1 -p 14000 &
sleep 5
#concoord replica -o server_test.serverTest -a 127.0.0.1 -p 14001 -b 127.0.0.1:14000 &
concoord acceptor -b  127.0.0.1:14000 &
#concoord replica -o concoord.object.counter.Counter -a 127.0.0.1 -p 14000 &
#concoord replica -o concoord.object.counter.Counter -a 127.0.0.1 -p 14001 -b127.0.0.1:14000 &

#sleep 10
#
#python run.py
#
#ps -A | grep "concoord" | awk '{print $1}' | xargs kill
