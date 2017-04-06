#!/usr/bin/env bash

#set -x


UPDATED="/tmp/wifi_updated.txt"
UPDATED_TGZ="wifi_update.tgz"
DELETED_TGZ="wifi_deleted.tgz"

#dir1 dir2 tags 
web_update_diff()
{
	local dir1 dir2 tags

	[ $# -lt 2 ] && echo "error: dir1 dir2 [tags]" && return 0

	# tags
	web_get_icbc_tgz()
	{
		local tags="packages_201406261610"
		local url=http://10.0.5.58/repos/packages/tags/$tags/icbc.tar.gz 

		if [ -n "$1" ] ; then
			url=$(echo $url | sed "s/$tags/$1")
		fi
		
		echo "svn export --username=taowuwen $url /tmp/icbc.tar.gz"
		svn export --username=taowuwen $url /tmp/icbc.tar.gz
		! [ -f "/tmp/icbc.tar.gz" ] && echo "fetch icbc from $url failed" &&return 1

		tar xvzf /tmp/icbc.tar.gz -C /tmp
		! [ -d "/tmp/icbc" ] && return 1

		mv -f /tmp/icbc $dir1

		find $dir1 -type d -iname ".svn" -exec rm -rf {} \;

		return 0

	}

	web_get_icbc()
	{
		local url=http://10.0.5.58/repos/locuswifi/branch/icbc

		svn co --username=taowuwen $url $dir2

		! [ -d "$dir2" ] && return 1

		find $dir2 -type d -iname ".svn" -exec rm -rf {} \;

		return 0
	}


	check_args()
	{
		if [ `expr length $dir1` -gt `expr length $dir2` ]; then
			case $dir1/ in
				$dir2/*) return 1 ;;
			esac
		else
			case $dir2/ in
				$dir1/*) return 1;;
			esac
		fi

		return 0
	}

	dir1=$(realpath $1)
	dir2=$(realpath $2)
	tags=$3

	echo "dir1=$dir1 dir2=$dir2, tags=$tags"

	check_args 
	! [ $? -eq 0 ]  && echo "self contained" && return 1

	! [ -d $dir1 ] && web_get_icbc_tgz $tags && ! [ $? -eq 0 ] && return 1
	! [ -d $dir2 ] && web_get_icbc && ! [ $? -eq 0 ] && return 1

	gen_diff()
	{
		shopt -s globstar

		for foo in $1/**
		do
			bar=$(echo $foo | sed "1s|$2|$3|")

			if [ -d "$foo" ]; then 
				! [ -d $bar ] && echo $foo  && echo $foo >>$UPDATED
			else 
				cmp "$foo" "$bar" &>/dev/null
				! [ $? -eq 0 ] && echo $foo && echo $foo >>$UPDATED
			fi
		done
	}

	# dir filename
	tar_diff()
	{
		local d f

		d=$1
		f=$2

		pushd $(pwd)
		cd $d
		tar cvzf /tmp/$UPDATED_TGZ $(cat $f | sed "s|$1/||")
		popd

		mv -f /tmp/$UPDATED_TGZ ./  &>/dev/null
	}

	gen_deleted()
	{
		shopt -s globstar

		for foo in $1/**
		do
			bar=$(echo $foo | sed "1s|$2|$3|")

			if [ -d "$foo" ]; then 
				! [ -d "$bar" ] && echo $foo && echo $foo | sed "1s|$1/||" >>$DELETED_TGZ
			else 
				! [ -f "$bar" ] && echo $foo && echo $foo | sed "1s|$1/||" >>$DELETED_TGZ
			fi
		done

		return 0
	}

	rm -rf $UPDATED &>/dev/null
	rm -rf $DELETED_TGZ &>/dev/null
	rm -rf $UPDATED_TGZ &>/dev/null

	gen_diff $dir2 $dir2 $dir1
	tar_diff $dir2 $UPDATED
	rm -rf $UPDATED &>/dev/null

	gen_deleted $dir1 $dir1 $dir2

	echo "done updated file: $UPDATED_TGZ, obsoluted file: $DELETED_TGZ"

	return 0
}

web_update_apply()
{
	local dir

	[ $# -lt 1 ] && echo "error: directory need to supply" && return 1

	dir=$(realpath $1)

	! [ -d "$dir" ] && echo "invalid directory $dir" && return 1

	update_existing()
	{
		tar xvzf $UPDATED_TGZ -C $dir
	}

	delete_obsolute()
	{
		cat $DELETED_TGZ | 
		while read line
		do
			echo "delete file $dir/$line"
			rm -rf $dir/$line
		done
	}

	echo "updating.."

	[ -f $UPDATED_TGZ ] && update_existing
	[ -f $DELETED_TGZ ] && delete_obsolute

	echo "finished"

	return 0
}


rsync_update()
{
	return 0
}



case $1 in
	web_diff)
		shift
		web_update_diff $*
	;;

	web_apply)
		shift
		web_update_apply $*
	;;

	rsync)
		shift
		rsync_update $*
	;;


	*)
		echo "usage: $0 web_diff/web_apply/rsync"
	;;
esac

exit $?
