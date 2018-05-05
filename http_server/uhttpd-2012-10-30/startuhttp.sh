#!/usr/bin/env bash

ROOT=${HOME}/codec/html

./uhttpd  -s 0.0.0.0:8880 -n 20 -T 10 -h $ROOT -x /cgi-bin -i .py=/usr/bin/python3 -I index.py -I index.html -I index.htm

#-i .lua=/usr/bin/lua -i .sh=/usr/bash -i .py=/usr/bin/python -f &
