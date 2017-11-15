#!/bin/sh

# name type version url
[ $# -lt 4 ] && echo "error; filename filetype filever fileurl" && return 1

target_dir="/tmp/app_intall"
currentdir=$(pwd)


prepare_env()
{
	mkdir -p $target_dir
	currentdir=$(pwd)
	cd $target_dir
}

clear_env()
{
	cd $currentdir
	rm -rf $target_dir
}

# version type
# file lists : appinfo setup
app_check_and_install()
{
	local appinfo setup ty ver chksum fname appdir

	appinfo="./appinfo"
	setup="./setup"

	! [ -f "$appinfo" -a -f "$setup" ] && echo "error $appinfo or $setup missing" && return 1

	ver=$(grep "app.version" $appinfo | awk -F= '{print $2}')
	ty=$(grep  "app.type" $appinfo | awk -F= '{print $2}')
	chksum=$(grep "app.checksum" $appinfo | awk -F= '{print $2}')
	fname=$(grep "app.filename" $appinfo | awk -F= '{print $2}')
	appdir=$(grep "app.name" $appinfo | awk -F= '{print $2}')

	! [ -f $fname ]  && echo "file $fname missing" && return 1

	crc=`md5sum $fname | cut -d ' ' -f 1`

	[ "$1" != "$ver" ] && echo "version not match, $1 != $ver" && return 1
	[ "$2" != "$ty"  ] && echo "type not match, $2 != $ty" && return 1
	[ "$chksum" != "$crc" ] && echo "checksum not match, $chksum != $crc" && return 1

	chmod u+x $setup

	tar xvzf $fname

	! [ -d "$appdir" ] && echo "directory $appdir missing" && return 1

	cp -f $setup $appinfo $appdir

	cd $appdir

	$setup install

	cd ../

	return $?
}

main()
{
	local ret name ty ver url target

	name=$1
	ty=$2
	ver=$3
	url=$4
	target="app_$2.tgz"
	ret=0

	app_download()
	{
		wget -c $url -O $target 2>&1
		ret=$?
	}

	app_download
	! [ $ret -eq 0 ] && echo "download: $url failed" && return $ret

	app_install()
	{
		local dst
		! [ -f $target ] && echo "$targe missing" && return 1

		dst="./app"

		mkdir -p $dst

		tar xvzf $target -C  $dst

		cd $dst
		app_check_and_install $ver $ty
		ret=$?
		cd ..
	}

	app_install

	return $ret
}

prepare_env
main $*
errcode=$?
clear_env

exit $errcode
