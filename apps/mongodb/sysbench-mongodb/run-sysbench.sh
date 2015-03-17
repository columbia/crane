#!/bin/bash

# simple script to run against running MongoDB/TokuMX server localhost:(default port)

# enable passing different config files

if [ ! $1 ];
then
	echo "Usage: ./run-sysbench <mongodb server IP> <mongdb server port>"
	exit 1;
fi

if [ ! $2 ];
then
        echo "Usage: ./run-sysbench <mongodb server IP> <mongdb server port>"
        exit 1;
fi



# Heming cleanup work.
FILE="config.bash"
echo "Loading config from $FILE....."
source $FILE;
MONGO_SERVER=$1
MONGO_PORT=$2


cd $MSMR_ROOT/apps/mongodb/sysbench-mongodb

echo "Running sysbench-mongodb benchmark..."
export CLASSPATH=$MSMR_ROOT/apps/mongodb/mongo-java-driver-2.13.0.jar:$CLASSPATH
javac -cp $CLASSPATH:$PWD/src src/jmongosysbenchload.java
javac -cp $CLASSPATH:$PWD/src src/jmongosysbenchexecute.java


# execute the benchmark

if [[ $DOQUERY = "yes" ]]; then
    echo Do query at $( date )
    export LOG_NAME=mongoSysbenchExecute-${NUM_COLLECTIONS}-${NUM_DOCUMENTS_PER_COLLECTION}-${NUM_WRITER_THREADS}.txt
    export BENCHMARK_TSV=${LOG_NAME}.tsv
 
    rm -f $LOG_NAME
    rm -f $BENCHMARK_TSV

# $SYSBENCH_INSERTS  $SEED 

    T="$(date +%s)"
    java -cp $CLASSPATH:$PWD/src jmongosysbenchexecute $NUM_COLLECTIONS $DB_NAME $NUM_WRITER_THREADS $NUM_DOCUMENTS_PER_COLLECTION $NUM_SECONDS_PER_FEEDBACK $BENCHMARK_TSV $SYSBENCH_AUTO_COMMIT $RUN_TIME_SECONDS $SYSBENCH_RANGE_SIZE $SYSBENCH_POINT_SELECTS $SYSBENCH_SIMPLE_RANGES $SYSBENCH_SUM_RANGES $SYSBENCH_ORDER_RANGES $SYSBENCH_DISTINCT_RANGES $SYSBENCH_INDEX_UPDATES $SYSBENCH_NON_INDEX_UPDATES $WRITE_CONCERN $MAX_TPS $MONGO_SERVER $MONGO_PORT $USERNAME $PASSWORD | tee -a $LOG_NAME
    echo "" | tee -a $LOG_NAME
    T="$(($(date +%s)-T))"
    printf "`date` | sysbench benchmark duration = %02d:%02d:%02d:%02d\n" "$((T/86400))" "$((T/3600%24))" "$((T/60%60))" "$((T%60))" | tee -a $LOG_NAME
fi

