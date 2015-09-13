#!/bin/sh
#
# Copyright (c) 2013,  Regents of the Columbia University 
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
# materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
#  TestRunner.sh - This script is used to run the deja-gnu tests exactly like
#  deja-gnu does, by executing the Tcl script specified in the test case's 
#  RUN: lines. This is made possible by a simple make target supported by the
#  test/Makefile. All this script does is invoke that make target. 
#
#  Usage:
#     TestRunner.sh {script_names}
#
#     This script is typically used by cd'ing to a test directory and then
#     running TestRunner.sh with a list of test file names you want to run.
#
TESTPATH=`pwd`
SUBDIR=""
if test `dirname $1` = "." ; then
  while test `basename $TESTPATH` != "test" -a ! -z "$TESTPATH" ; do
    tmp=`basename $TESTPATH`
    SUBDIR="$tmp/$SUBDIR"
    TESTPATH=`dirname $TESTPATH`
  done
fi

for TESTFILE in "$@" ; do 
  if test `dirname $TESTFILE` = . ; then
    if test -d "$TESTPATH" ; then
      cd $TESTPATH
      make check-one TESTONE="$SUBDIR$TESTFILE"
      cd $PWD
    else
      echo "Can't find tern/test directory in " `pwd`
    fi
  else
    make check-one TESTONE=$TESTFILE
  fi
done
