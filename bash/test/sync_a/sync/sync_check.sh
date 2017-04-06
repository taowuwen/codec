#!/usr/bin/env bash

check_and_start_master()
{
	pid=$(ps -aux | grep -i "inotifywait" | sed '/grep/d' | awk '{print $2}' | head -1)

	if ! [ $pid > 0 ]; then
		echo "$(date) master not running"
		/root/sync/be_master.sh
	else
		echo "$(date) master running $pid"
	fi
}

check_and_start_slave()
{
	pid=$(ps -aux | grep -i "rsync" | sed '/grep/d' | awk '{print $2}' | head -1)

	if ! [ $pid > 0 ]; then
		echo "$(date) slave not running"
		/home/tww/codecs/bash/sync/sync_check.sh
	else
		echo "$(date) slave running $pid"
	fi
}


main()
{
	case $1 in
		checkmaster*)
			shift
			check_and_start_master $*
		;;
		checkslave*)
			shift
			check_and_start_slave $*
		;;

		*)
			echo "Usage $0 checkmaster/checkslave"
		;;
	esac
}

main $*
exit $?
