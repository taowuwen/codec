#!/usr/bin/env bash


echo "current process id: $$"

kill_all_the_same()
{
	ps -aux | grep $0 | sed '/grep/d' | \
	while read line
	do
		pid=$(echo $line | awk '{print $2}' | sed "/$$/d")
		[ -n "$pid" ] && kill -9 $pid

	done

	sleep 10s
}

kill_all_the_same

