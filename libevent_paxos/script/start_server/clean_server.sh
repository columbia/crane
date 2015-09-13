#!/bin/bash
ps -A | grep "server.out" | awk '{print $1}' | xargs kill -9
rm -rf *.log
rm -rf .db
