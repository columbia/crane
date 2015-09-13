#! /bin/bash

ps -A | grep "server.out" | awk '{print $1}' | xargs kill
ps -A | grep "simple-server.o" | awk '{print $1}' | xargs kill

