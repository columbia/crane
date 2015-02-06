#!/bin/bash

shopt -s extglob
rm -rf !(*.cfg|*.py|readme.txt|clean_log.sh)
