#!/usr/bin/env sh

declare -a srvs

#set -x

#let i=1
#
#while [ $i -lt 100 ];
#do
#	wget  http://10.0.12.15/
#	let i=$($i+1)
#done

#servers="192.168.0.1 192.168.0.2 192.168.0.3"
#srvs=$(echo $servers | cut -f 1-99  -d ' ')


#dirname=$1
#echo ${dirname:?"nothing found"}

myreplace="/home/tww/aaaaa"
currentpath=$(realpath ./)

another_ver()
{
	shopt -s globstar

	for foo in $currentpath/**
	do
		echo $foo
	done
}


#another_ver
test_eval()
{
	local foo=10 x=foo

	#y='$'$x
	eval y='$'$x

	echo $y
}

test_eval


#echo "do sleep"
#nsecs=`expr $RANDOM % 10`
#echo "do sleep: $nsecs"
#sleep $nsecs
#echo "do sleep: $nsecs finished"
#sleep `expr $RANDOM % 10`





exit 0





# dir1 compare2
list_all_subdirs()
{
	local root=$(realpath $1)
	for foo in $(ls -a $root);
	do 
		if [ "$foo" = "."  -o "$foo" = ".." ]; then
			continue
		fi

		foo=$(realpath $foo)

		if [ -d $foo ]; then 
			echo "dir : $foo"
			list_all_subdirs $foo
		else 
			echo "$foo >>>>> "$(echo $foo | sed "s|$currentpath|$myreplace|")
		fi; 
	done
}

list_all_subdirs ./


exit 0


expr substr "hello, world" 1 5
expr length "hello, world"

srvs="
      192.168.0.1
      192.168.0.2
      192.168.0.3
      192.168.0.4
      "

for srv in $srvs; do
	echo "srv=$srv"
done

for arg in $*; do
	echo "arg=$arg"
done

do_test()
{
	echo TEST000args number= $# hello, world $@
	shift
	echo TEST000args number= $# hello, world $*
}

do_test1()
{
	echo test1------------args number= $# hello, world $@
	shift
	echo test1-----------args number= $# hello, world $*
}


echo "start to edit"
vim aaa

echo "finished edit"


do_test $@
do_test1 $@


echo args number= $# hello, world $@
echo args number= $# hello, world $*

shift
if [ $# -lt 2 ] ; then
	exit 0
fi


echo args number= $# hello, world $@
echo args number= $# hello, world $*

shift
if [ $# -lt 2 ] ; then
	exit 0
fi

echo args number= $# hello, world $@
echo args number= $# hello, world $*


shift
if [ $# -lt 2 ] ; then
	exit 0
fi

echo args number= $# hello, world $@
echo args number= $# hello, world $*



