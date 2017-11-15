#!/bin/sh


export PATH=$PATH:/tmp/ems/bin

while true
do
	local ems_st=$(which ems_status)

	[ -n "$ems_st" -a -h "$ems_st" ] && {
		ret=`$ems_st flag=1; echo $?`
		
		[ $ret == 255 ] && {
			echo "$(date): emsd did not response, do restart" >>/tmp/ems_restart.log
			/etc/init.d/emsd start
		}
	}

	sleep 30s
done


