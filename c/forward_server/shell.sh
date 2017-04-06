#!/usr/bin/env bash

set -x

#servers

rm /tmp/fwd_logger.log

#default_args="-s 10.0.12.64 -p 80"
#default_args="-s 222.161.197.95 -p 80"
#default_args="-s localhost -p 80"
default_args="-s www.bing.com -p 80"

arg_1="redirect.php"
arg_2="fuqqtest/test.jpg"
arg_3="index.php"
arg_4="indexs.html"
arg_5="index.php?m=homepage&a=index&user=gongzonghang"
arg_6="index.html"

./fwd_cm_testing $default_args -g $arg_6 $@

