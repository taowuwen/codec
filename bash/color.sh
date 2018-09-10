#!/usr/bin/env bash


if [ -z $1 ]; then
	BREAK=8
else
	BREAK=$1
fi

version_one()
{
	i=0

	while [ $i -lt 255 ];
	do
		tput setaf $i
		printf "\x1b[38;5;${i}mcolour${i} \t"
		if [ $(( i % $BREAK )) -eq $(($BREAK-1)) ] ; then
			printf "\n"
		fi

		i=`expr $i + 1`
	done
}



version_two()
{
# 0-8
# 30-37
# 40-47
#	[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ 

	local i j k

	i=0

	while [ $i -le 8 ];
	do
		j=30
		while [ $j -le 37 ];
		do
			k=40

			while [ $k -le 47 ];
			do
				echo -n "\033[0$i;$j;$k"m" $i;$j;$k \033[00m"

				k=`expr $k + 1`
			done
			j=`expr $j + 1`

			echo ""
		done
		i=`expr $i + 1`
	done
}

#version_one
version_two
