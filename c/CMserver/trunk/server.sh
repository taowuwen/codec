#!/bin/sh
#***********************************************
# File Name  : server.sh
# Description: 
# Make By    :lqf/200309129@163.com
# Date Time  :2012/07/13 
#***********************************************

UsAge="UsAge:\"./dtServer.sh start\" or \"./dtServer.sh stop\" or \"./dtServer.sh status\""

space=""
ParamInput="$1"

if [ "start" = "$ParamInput" ]
then
    chmod 777 dtServer
    chmod 777 Monitor.sh

    servername=`ps -aef | grep dtServer | grep -v grep | awk '{print $NF}'`
    if [ "./dtServer" != "$servername" ]
    then
        ./dtServer
        echo started dtServer
    fi

    servername=`ps -aef | grep Monitor.sh | grep -v grep | awk '{print $NF}'`
    if [ "./Monitor.sh" != "$servername" ]
    then
        ./Monitor.sh &
        echo started monitor
    fi
    
    exit 0
fi

if [ "status" = "$ParamInput" ]
then
    servername=`ps -aef | grep dtServer | grep -v grep | awk '{print $NF}'`
    if [ "./dtServer" != "$servername" ]
    then
        echo dtServer not started.
    else
        echo dtServer started.
    fi
   
    servername=`ps -aef | grep Monitor.sh | grep -v grep | awk '{print $NF}'`
    if [ "./Monitor.sh" != "$servername" ]
    then
        echo monitor not started.
    else
        echo monitor started.
    fi

    exit 0
fi

if [ "stop" = "$ParamInput" ]
then
    pid=`ps -aef | grep Monitor.sh | grep -v grep | awk '{print $2}'`
    if [ "$space" != "$pid" ]
    then
        kill -9 $pid >/dev/null  2>&1
    fi

    pid=`ps -aef | grep dtServer | grep -v grep | awk '{print $2}'`
    if [ "$space" != "$pid" ]
    then
        kill -9 $pid >/dev/null  2>&1
    fi

    exit 0
fi

echo $UsAge
