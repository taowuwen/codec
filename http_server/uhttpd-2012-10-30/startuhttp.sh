#!/usr/bin/env bash


./uhttpd  -s 0.0.0.0:80 -n 20 -T 10 -h /home/tww -x /cgi-bin -f &

#-i .lua=/usr/bin/lua -i .sh=/usr/bash -i .py=/usr/bin/python -f &
