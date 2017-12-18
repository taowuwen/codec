#!/usr/bin/env bash


echo ""

n=1
while [ $n -lt 101 ];
do
	mod_3=`expr $n \% 3`
	mod_5=`expr $n \% 5`

	[ $mod_3 -eq 0 -o $mod_5 -eq 0 ] && {

		if [ $mod_3 -eq 0 -a $mod_5 -eq 0 ]; 
		then
			echo -n " Fizz-Buzz"
		elif [ $mod_3 -eq 0 ] ; then
			echo -n " Fizz"
		else
			echo -n " Buzz"
		fi

	} || {
		echo -n " $n"
	}

	n=`expr $n + 1`
done

echo ""

