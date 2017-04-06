#!/usr/bin/env bash


[ $# -lt 1 ] && echo "error args: directory" && exit 0
! [ -d $1 ] && echo "error $1 not a directory" && exit 0


INOTIFYWAIT=$(which inotifywait)


do_sync()
{
	local cnt evt

	wait_for_evt()
	{
		$INOTIFYWAIT  -t 5 -rq --timefmt '%d/%m%y %H%M' --format '%T %w %f %e' \
			-e delete,create,attrib,move,delete_self,close_write $1

		case $? in 
			0)
				evt=1
				cnt=2
			;;

			2)
				evt=0 # do not handle cnt now
#				cnt=`expr $cnt - 1`
			;;

			*)
				evt=0
				cnt=0
			;;
		esac

		return $?
	}


	cnt=1
	evt=0
	while true
	do
		wait_for_evt $1

		if [ $cnt -gt 0 ];  then
			echo "do sync..."
			cnt=`expr $cnt - 1`
		fi
	done
}

do_sync $*

#$INOTIFYWAIT -t 5 -mrq --timefmt '%d/%m%y %H%M' --format '%T %w %f %e %Xe' \
#	-e modify,delete,create,attrib,move,delete_self $1 | \

#$INOTIFYWAIT  -mrq --timefmt '%d/%m%y %H%M' --format '%T %w %f %e' \
#	-e delete,create,attrib,move,delete_self,close_write $1 | \
#while read files
#do
#	echo "$? $files"
#done
#

#cnt=1
#	let cnt=$cnt+1

#while [ $cnt -lt 100 ]
#do
#	echo cnt = $cnt
#
#	cnt=`expr $cnt + 1`
#done
