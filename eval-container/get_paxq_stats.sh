#!/bin/bash
# please run this script on bug 03

connect_num=$(cat ~/server-out.txt | grep pop_front | grep PAXQ_CONNECT | wc -l)
echo PAXQ_CONNECT number: $connect_num
send_num=$(cat ~/server-out.txt | grep pop_front | grep PAXQ_SEND | wc -l)
echo PAXQ_SEND number: $send_num
close_num=$(cat ~/server-out.txt | grep pop_front | grep PAXQ_CLOSE | wc -l)
echo PAXQ_CLOSE number: $close_num

# count PAXQ_NOP before the first PAXQ_CONNECT
first_connect_line=$(grep -n -m 1 PAXQ_CONNECT ~/server-out.txt | cut -f1 -d: )
nop_before_connect=$(head -n `expr $first_connect_line - 1` ~/server-out.txt | grep PAXQ_NOP | wc -l)
echo PAXQ_NOP number before first PAXQ_CONNECT: $nop_before_connect

# count PAXQ_NOP between the first PAXQ_CONNECT and the last PAXQ_CLOSE
last_close_line=$(grep -n -m $close_num PAXQ_CLOSE ~/server-out.txt | tail -n 1 | cut -f1 -d: ) 
# echo $last_close_line
echo PAXQ_NOP number between first PAXQ_CONNECT and last PAXQ_CLOSE: `sed -n "$first_connect_line,$last_close_line p" ~/server-out.txt | grep PAXQ_NOP | wc -l`
