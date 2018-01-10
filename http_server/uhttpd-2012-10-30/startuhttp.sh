#!/usr/bin/env bash

ROOT=${HOME}/codec/html

./uhttpd  -s 0.0.0.0:80 -n 20 -T 10 -h /home/tww -x /cgi-bin

#-i .lua=/usr/bin/lua -i .sh=/usr/bash -i .py=/usr/bin/python -f &
