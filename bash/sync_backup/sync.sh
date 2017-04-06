#!/usr/bin/env sh

#set -x

RSYNC=$(which rsync)
INOTIFYWAIT=$(which inotifywait)

RSYNCPASS="mp_rsync_pass"
RSYNCUSER="web"
RSYNCSECRET="sync_secret"


RSYNC_SERVERS="
	172.16.127.10
	172.16.127.20
	172.16.127.30
	"

SYNC_PORT=10000

RSYNC_CONF="rsyncd.conf"
RSYNC_CLIENT="$(pwd)/start_client.sh"

client_create_pass()
{
	echo "$RSYNCUSER:$RSYNCPASS" > $RSYNCSECRET
	chmod 640 $RSYNCSECRET
}

server_create_pass()
{
	echo "$RSYNCPASS" > $RSYNCSECRET
	chmod 640 $RSYNCSECRET
}

start()
{
	web="/tmp/www"
	servs="172.16.127.10"
	password="$(pwd)/pwd"
	exclude="$(pwd)/exclude"


	$INOTIFYWAIT -mrq --timefmt '%d/%m%y %H%M' --format '%T %w%f%e' \
		-e modify,delete,create,attrib,move,delete_self $web | \
	while read files
	do
		for serv in $servs; do
			$RSYNC -vaz --delete --progress $web \
				--exclude-from=$exclude	\
				--password-file=$password web@$serv::web --port=10000

		done
	done

	return 0
}


client_gen_rsync_conf()
{
	if ! [ -f $RSYNC_CONF ]; then
		echo "" > $RSYNC_CONF
		echo "uid=$USER" >> $RSYNC_CONF
		echo "gid=$USER" >> $RSYNC_CONF
		echo "" >> $RSYNC_CONF
		echo "use chroot = no" >> $RSYNC_CONF
		echo "max connections=10" >> $RSYNC_CONF
		echo "pid file = /tmp/rsyncd.pid" >> $RSYNC_CONF
		echo "lock file= /tmp/rsyncd.lock" >> $RSYNC_CONF
		echo "log file = /tmp/rsyncd.log" >> $RSYNC_CONF
		echo "" >> $RSYNC_CONF
		echo "[web]" >> $RSYNC_CONF
		echo "	comment = web files" >> $RSYNC_CONF
		echo "	ignore errors = no" >> $RSYNC_CONF
		echo "	read only = no" >> $RSYNC_CONF
		echo "	write only = no" >> $RSYNC_CONF
		echo "	list = false" >> $RSYNC_CONF
		echo "	auth users=web" >> $RSYNC_CONF
		echo "	secrets file = $(pwd)/$RSYNCSECRET" >> $RSYNC_CONF
		echo "	hosts allow = 172.16.127.10/24" >> $RSYNC_CONF
		echo "	hosts deny  = *" >> $RSYNC_CONF
		echo "	uid = $USER" >> $RSYNC_CONF
		echo "	gid = $USER" >> $RSYNC_CONF
		echo "	path = /home/wwwroot/default" >> $RSYNC_CONF
	else
		echo "$RSYNC_CONF existed already"
	fi
}

client_gen_start_files()
{
	if ! [ -f $RSYNC_CLIENT ]; then
		echo "#!/usr/bin/env sh" >>$RSYNC_CLIENT
		echo "killall rsync &>/dev/null" >>$RSYNC_CLIENT
		echo "rm -rf /tmp/rsyncd.* ">>$RSYNC_CLIENT
		echo "rsync --daemon --config=$RSYNC_CONF --port=$SYNC_PORT $@">>$RSYNC_CLIENT
		chmod u+x $RSYNC_CLIENT 
	fi

	return 0
}

be_client()
{
	echo "we are client"
	client_create_pass
	client_gen_rsync_conf $*
	client_gen_start_files $*

	echo "config ready, please open $RSYNC_CONF and edit by hand"
	echo "run $RSYNC_CLIENT + extra args --verbose gonna be a good idea"

	return 0
}


#root extra args
be_server()
{
	if [ $# -lt 1 ]; then
		echo "master sync root should supplied"
	fi

	root=$1
	server_create_pass
	sync_one $@

#	shift
#	--exclude-from=$exclude
	$INOTIFYWAIT -mrq --timefmt '%d/%m%y %H%M' --format '%T %w%f%e' \
		-e modify,delete,create,attrib,move,delete_self $root | \
	while read files
	do
		sync_one $@
#		for serv in $RSYNC_SERVERS; do
#			$RSYNC -az --delete --progress $root --port=$SYNC_PORT $@ \
#				--password-file=$RSYNCSECRET web@$serv::web
#		done
	done

	return 0
}

do_test()
{
	echo "do testing."
	start $*
}

#args
sync_one()
{
#	--exclude-from=$exclude	# if you want to exclude
	root=$1
	shift

	for serv in $RSYNC_SERVERS; do
		$RSYNC -az --delete --progress $root --port=$SYNC_PORT $@ \
			--password-file=$RSYNCSECRET web@$serv::web 

	done
}


main()
{
	case $1 in
		beclient*)
			shift
			be_client $*
		;;
		beserver*)
			shift
			be_server $*
		;;

		syncnow*)
			shift
			sync_one $*
		;;

		test)
			shift
			do_test $*
		;;
		*)
			echo "Usage $0 beclient/beserver/syncnow"
		;;
	esac
}

main $*
exit $?
