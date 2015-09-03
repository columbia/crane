#!/bin/bash

cnt=3

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
	time $MSMR_ROOT/apps/mysql/mysql-install/bin/mysql -u root -h $1 -P $2 -e \
		'use sysbench_db; select count(*) from sbtest where c REGEXP "3*7*1*6*76*3*0*7*";exit;' &
	(( i++ ))
done

sleep 3;
killall -9 mysql
exit 0
