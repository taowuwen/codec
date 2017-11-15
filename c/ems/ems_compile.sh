#!/usr/bin/env sh

do_patch()
{
	patch_dir="$(pwd)/patch"

	[ -d "$patch_dir" ] && {

		fls=$(ls $patch_dir)
		
		for foo in $fls
		do
			[ -n "`echo $foo | sed -n '/^[0-9]\{2,4\}.*\.patch$/p'`" ] && {
				echo "do patching $patch_dir/$foo"
				patch -p0 <$patch_dir/$foo
			}
		done
	}
}

case $1 in 
	patch)
		shift
		do_patch
	;;

	compile)
		shift
		make $*
	;;

	*)
		do_patch
		make $*
	;;
esac

