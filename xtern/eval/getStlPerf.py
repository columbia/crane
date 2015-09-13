#!/usr/bin/env python

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

import os
import argparse
import sys

def checkExist(file, flags=os.F_OK):
    if not os.path.exists(file) or not os.access(file, flags):
        return False
    return True

def getResult(directory, app, size, machine, num):
    i = 0
    annot = "@"
    if app.startswith("partition") or app.startswith("partial_sort") or app.startswith("nth_element"):
        annot = "N"
    if app.endswith('0'):
        app = app[:-3]
        annot = " "
    elif app.endswith('1'):
        app = app[:-3]
        if annot.startswith('N'):
            pass
        else:
            annot = "@"
    else:
        pass
    file = directory + '/stats.txt'
    if checkExist(file):
        for line in open(file):
            i += 1
            if i == 1:
                stub1, overhead = line.strip().split(' ')
            elif i == 4:
                stub1, xtern_avg = line.strip().split(' ')
            elif i == 5:
                stub1, xtern_sem = line.strip().split(' ')
            elif i == 7:
                stub1, nondet_avg = line.strip().split(' ')
            elif i == 8:
                stub1, nondet_sem = line.strip().split(' ')
            else:
                pass
        print '\t'.join([app, size, machine, num, annot, 
            nondet_avg, nondet_sem, xtern_avg, xtern_sem, overhead])
    else:
        print file + ' not existed yet'

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Collect stl performance number for excel')
    parser.add_argument('dirname',
        type = str,
        help = 'directory containing the stl result (e.g. current)') 
    args = parser.parse_args()
    if args.dirname.__len__() == 0:
        print 'Please specify a directory!'
        sys.exit(1)        
    else:
        directory = os.path.abspath(args.dirname)
        if not checkExist(directory):
            print 'Invalid directory name'
        else:
            for name in os.listdir(directory):
                temp_name = name.strip().replace("stl_", "")
                getResult(os.path.abspath(directory+'/'+name), temp_name, 
                    "large", "bug01", "24")
