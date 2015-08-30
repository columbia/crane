#!/bin/bash

./py_http.py

LD_PRELOAD=$MSMR_ROOT/libevent_paxos/client-ld-preload/libclilib.so ./http_request.py
