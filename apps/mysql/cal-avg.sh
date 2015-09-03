#!/bin/bash
AVG=`cat $1 | grep real | awk '{ sum += $1; n++ } END { if (n > 0) print sum / n; }'`
echo "Average time: $AVG";
