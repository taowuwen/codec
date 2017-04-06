#!/bin/sh
#***********************************************
# File Name  : Monitor.sh
# Description: 
# Make By    :lqf/200309129@163.com
# Date Time  :2012/07/13 
#***********************************************
space=""
sleeptime=5

while [ 1 ]
do
    sleep $sleeptime
    servernam=`ps -aef | grep dtServer | grep -v grep | awk '{print $NF}'`
    if [ "./dtServer" != "$servernam" ]
    then
        ./dtServer
    fi
done
