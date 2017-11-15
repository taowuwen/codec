#!/bin/sh


#set -x

# for escape string  cat test.txt | sed -n 's|\([^a-zA-Z0-9]\)|\\\1|gp'

FTP_PORT=""
FTP_ADDR=""
FTP_USER=""
FTP_PASS=""
FTP_HOME=""
cfg=""

# string
escape_string()
{
	echo $1 | sed 's|\([^a-zA-Z0-9_]\)|\\\1|g'
#	echo $1 | sed 's|\([&(){}[]]\)|\\\1|g'
}


update_ftp_params()
{
	FTP_USER=`grep out.e6wifi.ftp.user $cfg | awk -F= '{print $2}'`
	FTP_PASS=`grep out.e6wifi.ftp.pass $cfg | awk -F= '{print $2}'`
	FTP_ADDR=`grep out.e6wifi.ftp.addr $cfg | awk -F= '{print $2}'`
	FTP_PORT=`grep out.e6wifi.ftp.port $cfg | awk -F= '{print $2}'`
	FTP_HOME=`grep out.e6wifi.ftp.home $cfg | awk -F= '{print $2}'`
}

do_upload_file()
{
	local user pass addr port home fl path

	fl=$1
	path=$2

	while true
	do
		update_ftp_params

		wput -t 0 $fl ftp://$FTP_USER:$FTP_PASS@$FTP_ADDR:$FTP_PORT$FTP_HOME/$path
		err=$?

		# err == 0, success
		# err == 1, some files were skipped during the upload
		# err == 2, remote err
		# err == 3, some file failed and other skipped
		# err == 4, general problem. e.g. system call failed

		[ $err -eq 0 ] && break
	done

	return $err
}

# dir filename
upload_file()
{
	local pre fl fl_zip err flname apmac curdir


	apmac=$3
	curdir=$(pwd)

	cd `dirname $2`

	{
		pre=$1
		fl=`basename $2`
		fl_zip=$fl.zip

		zip -9 -m $fl_zip `basename $fl`

		! [ -f "$fl_zip" ] && return 1

		do_upload_file $fl_zip $apmac/internet/$pre/$fl".tmp"

		err=$?

		[ $err -eq 0 ] && {
cat <<EOF | ftp -n
verbose
open $FTP_ADDR $FTP_PORT
user $FTP_USER $FTP_PASS
passive
binary
cd $FTP_HOME/$apmac/internet/$pre
rename $fl".tmp" $fl_zip
bye
EOF
		}
	#mput test.txt
		rm -f $fl_zip
	}
	cd $curdir

	return $err
}



# well, we need 
# address port user pass home file
# 

handle_one()
{
	local fl prefix lastfl err apmac
	
	awk '
	BEGIN {
		total = 0
		rootdir="/tmp"	
		newfile=""
		prefix=""
		filename=rootdir"/lastfileinfo"
		srand()
		apmac = ""
		FS=","
	}

	function getfilename(line)
	{
		split(line, ary, " ")
		tm = ary[2]
		prefix= ary[1]
		gsub(":", "-", tm)

		newfile=rootdir"/"prefix"-"tm"-"int(rand() * 1000)".internet"
	}

	{
		if (NF <= 3) pass

		if (newfile == "") {
			getfilename($3)
			apmac = $2
			gsub(":", "-", apmac)
			print(prefix, newfile, apmac) >filename
			close(filename)
		}

		if (!match($3, prefix)) exit

		print $0 >newfile

		total++
	}

	END {
		close(newfile)
		printf("total write lines: %d\n", total)
	}
	' $1

	lastfl="/tmp/lastfileinfo"

	prefix=`cat $lastfl | awk '{print $1}'`
	fl=`cat $lastfl | awk '{print $2}'`
	apmac=`cat $lastfl | awk '{print $3}'`

	echo "prefix: $prefix, filename: $fl"

	rm -f $lastfl

	# delete unused 
	sed -i "/$prefix/d" $1

	# do upload 
	upload_file $prefix $fl $apmac
	err=$?

	# delete file
	rm -f $fl

	return $err
}


# for now, just update file

tmp_update_file()
{
	awk '
	BEGIN {
		total = 0
		rootdir="/tmp"	
		newfile="/tmp/awk_tmp_file.log"
		FS=","
	}

	function audit_get_date(line) {
		split($1, n, ".")

		cmd="date -d @"n[1]" +\"%F %H:%M:%S\""

		cmd | getline data
		close(cmd)

		return data
	}

	{
		printf("%s,%s\n",audit_get_date($0), $0) >newfile
		total++
	}

	END {
		close(newfile)
		printf("total write lines: %d\n", total)
	}

	' $1

	mv /tmp/awk_tmp_file.log $1
}

# fl, ftp(user, pass, addr, port, home)
# fl, cfg_file
main()
{
	local fl err

	[ $# -lt 2 ] && echo "error: file or config file missing" && return 1

	fl=$1
	cfg=$2

	! [ -f $fl -a -f $cfg ] && echo "file $fl or $cfg not exist" && return 1

	sed -i '/.*,.*,[0-9]\{4\}\(-[0-9]\{2\}\)\{2\}/!d' $fl

	err=0
	while true
	do
		[ `ls -l $fl | awk '{print $5}'` -le 0 ] && break
		handle_one $fl
		err=$?
	done

	rm -f $fl

	return $err
}

main $*
exit $?
