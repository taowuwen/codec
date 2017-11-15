#!/usr/bin/env bash

#set -x

[ $# -lt 1 ] && echo "error, missing directory" && exit 1

main()
{
	local objdir appinfo setup root appname appver app root
		
	objdir=$(readlink -f $1)

	# directory 
	check_input()
	{
		local sz
		! [ -d "$objdir" ] && echo "error, $objdir  not a directory" && return 1
		
		sz=$(du -s $objdir | awk '{print $1}')

		max=7168
		[ $sz -ge $max ] && echo "$objdir too large, $max at most" && return 1

		appinfo="$objdir/appinfo"
		setup="$objdir/setup"

		! [ -f "$appinfo" -a -f "$setup" ] && echo "$appinfo or $setup missing" && return 1

		appname=$(grep app.name $appinfo | awk -F= '{print $2}')

		! [ -n "$appname" ] && echo "appname missing" && return 1

		return 0
	}


	check_input
	! [ $? -eq 0 ] && return 1

	root="/tmp/ems_pack"


	do_pack()
	{
		local ver pkg

		cp ./$appname/appinfo ./
		cp ./$appname/setup   ./

		pkg=./$appname.tgz
		tar cvzf $pkg $appname

		! [ -f "$pkg" ] && echo "tar failed" && return 1

		sed -i '/checksum/d' appinfo
		echo "app.checksum=`md5sum $pkg | cut -d ' ' -f 1`" >> appinfo

		sed -i '/app.filename/d' appinfo
		echo "app.filename=$pkg" >> appinfo

		ver=$(grep "app.version" appinfo | awk -F= '{print $2}')
		! [ -n "$ver" ] && ver="1.0.0.0"

		app=$root/$appname.$ver.tar.gz

		tar cvzf $app ./appinfo ./setup $pkg
	}


	mkdir -p $root &>/dev/null

	pushd $(pwd)
	cd $root

	mkdir $appname
	cp -rf $objdir/* $appname/

	do_pack

	popd

	if [ -f "$app" ]; then 
		cp -f $app ./
		echo "sucess"
	else
		echo "failed"
	fi

	rm -rf $root

	return 0
}

main $*
