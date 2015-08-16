#!/bin/bash

rm -rf scheds
mkdir scheds
scp bug03.cs.columbia.edu:/dev/shm/dmt_out/serializer-light-pid-*.log scheds/bug03.log
scp bug01.cs.columbia.edu:/dev/shm/dmt_out/serializer-light-pid-*.log scheds/bug01.log
scp bug02.cs.columbia.edu:/dev/shm/dmt_out/serializer-light-pid-*.log scheds/bug02.log
