#!/bin/sh

#set -x
gen_random()
{
	awk '
		BEGIN {
			srand()
			ra=rand() * 1000000
		}
		{
			exit
		}
		END {
			printf("%d", ra)
		}
	' $0
}

# size chr
fill_buf()
{
	local sz num i buf

	sz=$1
	num=$2

	buf=""

	i=0
	while [ 1 -eq `expr $i \< $sz` ];
	do
		buf=$buf"\\x$num"
		i=`expr $i + 1`
	done

	echo $buf
}


#name size number
fill_file_v1()
{
	local fl sz num cur

	[ $# -lt 3 ] && echo "filename size chr missing" && return 1
	
	fl=$1
	sz=$2
	num=$3
	buf="\\x$num"

	cur=1

	while [ 1 -eq `expr $cur \<= $sz` ];
	do
		[ 0 != $((cur & sz)) ] && {
			echo "total size: $sz: write size: $cur"
			echo -ne $buf >>$fl
		}
		buf=$buf$buf
		cur=$((cur << 1))
	done
}


check_one()
{
	local sz=$1
		
	tmpfl="/tmp/aaa.dat"

	[ -f $tmpfl ] && {
		rm  $tmpfl ||  {
			echo "remove $tmpfl failed" 
			exit 1 
		}
	}

	echo "test size $sz"

	fill_file_v1 $tmpfl $sz "00"
}

test()
{
	local sz total wrong right

	let sz=1024*1024*20

	total=1
	wrong=0
	right=0

	while [ 1 -eq $((total > 0)) ];
	do
		echo "total = $total"
		total=`expr $total - 1`

		check_one $sz

		chksum=`md5sum $tmpfl | awk '{print $1}'`
#	ff	3m=498e23c82ca78a4bcd7ba88e5243b6fc
#	ff	4m=2b7a70fa59f8173635bcbe956bad56c6
#	00	3m=d1dd210d6b1312cb342b56d02bd5e651
#	00	20m=8f4e33f3dc3e414ff94e5fb6905cba8c

		if [ "$chksum" != "8f4e33f3dc3e414ff94e5fb6905cba8c" ]; then
			wrong=$((wrong + 1))
			echo "warning, memory error, current $chksum"
		else
			right=$((right + 1))
		fi

	done

	total=`expr $right + $wrong`
	rate=`expr $right / $total`
	rate=$((rate * 100))
	echo "done: total $total, right: $right, wrong: $wrong right rate: $rate%"
}

test

