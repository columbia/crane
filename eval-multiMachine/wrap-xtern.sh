#!/bin/bash
# Heming: this script wrap xtern's ld-preload library and faithfully execute it
# with the full command line from this script. This script is for valgrind to execute with xtern.

LD_PRELOAD=$XTERN_ROOT/dync_hook/interspose.so $*
