#!/usr/bin/env bash

ROOT=${HOME}/usr/html

bind_address="-s 0.0.0.0:8080"

[ `id -u` -eq 0 ] && bind_address="$bind_address -s 0.0.0.0:80"

./uhttpd  $bind_address -n 20 -T 10 -h $ROOT -x /cgi-bin -i .py=/usr/bin/python3 -I index.py -I index.html -I index.htm

#-i .lua=/usr/bin/lua -i .sh=/usr/bash -i .py=/usr/bin/python -f &
