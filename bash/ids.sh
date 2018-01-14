#!/usr/bin/env bash


#log="/home/tww/auth.log"
log="/var/log/auth.log"

main() {
	cd ${HOME}

	cat $log |
	awk '/Disconnecting: Too many authentication failures/{print $(NF-4)}' |
	uniq |
	tail -20 |
	sort |
	uniq >invalid.txt

	iptables -L -vn

	n=`cat invalid.txt | wc -l`

	[ $n -gt 0 ] && {
		mv invalid.txt invaliduser.txt
		cat invaliduser.txt
		./invalid_target.sh
		echo "" >$log
	}

}

_do_start() {
	echo "start to checking.... `date`"
	main $*
	echo "checking finished `date`"
} >>/tmp/ids.log

_do_start
