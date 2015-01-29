#!/bin/bash

shopt -s extglob
rm -rf !(*.cfg|eval.py|readme.txt|clean_log.sh)
