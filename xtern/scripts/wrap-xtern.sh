#!/bin/bash
# Heming: this script wrap xtern's ld-preload library and faithfully execute it
# with the full command line from this script. This script is for valgrind to execute with xtern.

BASEDIR=$(dirname $0)
LD_PRELOAD=$BASEDIR/../dync_hook/interpose.so eval "$*"
