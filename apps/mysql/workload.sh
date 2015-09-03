#!/bin/bash

cnt=5
STR="3*7*1*6*76"

if [ ! $1 ];
then
        echo "Usage: $0 <server IP> <server port>"
        echo "$0 127.0.0.1 7000"
        exit 1;
fi

if [ ! $2 ];
then
        echo "Usage: $0 <server IP> <server port>"
        echo "$0 127.0.0.1 7000"
        exit 1;
fi


i=1
while [ $i -le $cnt ]
do
	/usr/bin/time -f "%e real" ./mysql-install/bin/mysql -u root -h $1 -P $2 -e \
		'use sysbench_db; select count(*) from sbtest where c REGEXP "$STR";' &
	sleep 0.001
	(( i++ ))
done

sleep 5;
killall -9 mysql
exit 0
