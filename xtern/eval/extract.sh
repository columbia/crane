#!/bin/bash

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

if [ ! -d $XTERN_ROOT ]; then
    echo "XTERN_ROOT is not defined"
    exit 1
fi

if [ "$1" == "" ]; then
    logs=$XTERN_ROOT/eval/current
else
    logs=$1
fi

if [ ! -d $logs ]; then
    echo "$logs is not a folder!"
    exit 1
fi
cd $logs

find . -name stats.txt -print | sort | cut -d "/" -f 2 

find . -name stats.txt -print | sort | xargs -I {} bash -c \
"awk '
BEGIN{line=0;
      dthreads=0; dmp_o=0; dmp_b=0; dmp_pb=0; dmp_hb=0;
      avg_count=0; sem_count=0}
{
    if (\$1 == \"dthreads:\") dthreads = 1;
    if (\$1 == \"dmp_o:\") dmp_o = 1;
    if (\$1 == \"dmp_b:\") dmp_b = 1;
    if (\$1 == \"dmp_pb:\") dmp_pb = 1;
    if (\$1 == \"dmp_hb:\") dmp_hb = 1;
    if (\$1 == \"avg\") {
        avg[avg_count] = \$2;
        ++avg_count;
    }
    if (\$1 == \"sem\") {
        sem[sem_count] = \$2;
        ++sem_count;
    }
    #line++;
    #if (line == 4) xtern_avg = \$2;
    #if (line == 5) xtern_sem = \$2;
    #if (line == 7) nondet_avg = \$2;
    #if (line == 8) nodet_sem = \$2;
    #if (line == 8){print nondet_avg, nodet_sem, xtern_avg, xtern_sem;}
}
END {
    printf \"%s %.10f %s %.10f\", avg[2], sem[1], avg[1], sem[0]
    avg_count = 4
    sem_count = 2
    if (dthreads == 1) {
        printf \" %s %.10f\", avg[avg_count], sem[sem_count];
        avg_count += 2;
        sem_count += 1;
    }
    else
        printf \"  \";
    if (dmp_o== 1) {
        printf \" %s %.10f\", avg[avg_count], sem[sem_count];
        avg_count += 2;
        sem_count += 1;
    }
    else
        printf \"  \";
    if (dmp_b == 1) {
        printf \" %s %.10f\", avg[avg_count], sem[sem_count];
        avg_count += 2;
        sem_count += 1;
    }
    else
        printf \"  \";
    if (dmp_pb == 1) {
        printf \" %s %.10f\", avg[avg_count], sem[sem_count];
        avg_count += 2;
        sem_count += 1;
    }
    else
        printf \"  \";
    if (dmp_hb == 1) {
        printf \" %s %.10f\", avg[avg_count], sem[sem_count];
        avg_count += 2;
        sem_count += 1;
    }
    else
        printf \"  \";

    print \"\"
}
' {}; "

