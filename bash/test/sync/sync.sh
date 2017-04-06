#!/usr/bin/env bash

#set -x

RSYNC=$(which rsync)
INOTIFYWAIT=$(which inotifywait)

RSYNCPASS="mp_rsync_pass"
RSYNCUSER="web"
RSYNCSECRET="$(pwd)/sync_secret"


RSYNC_SERVERS="
	172.16.127.20
	172.16.127.30
	"

SYNC_PORT=10000

RSYNC_CONF="$(pwd)/rsyncd.conf"
RSYNC_CLIENT_NAME="start_slave.sh"
RSYNC_CLIENT="$(pwd)/$RSYNC_CLIENT_NAME"

RSYNC_CHECK="$(pwd)/sync_check.sh"

slave_create_pass()
{
	echo "$RSYNCUSER:$RSYNCPASS" > $RSYNCSECRET
	chown $USER:$USER $RSYNCSECRET
	chmod 640 $RSYNCSECRET
}

master_create_pass()
{
	echo "$RSYNCPASS" > $RSYNCSECRET
	chown $USER:$USER $RSYNCSECRET
	chmod 640 $RSYNCSECRET
}
# rsync master
slave_gen_rsync_conf()
{
	if ! [ -f $RSYNC_CONF ]; then

#create rsyncd.conf for deamon

cat <<EOF > $RSYNC_CONF
uid=$USER
gid=$USER

use chroot = no
max connections=10
pid file = /tmp/rsyncd.pid
lock file= /tmp/rsyncd.lock
log file = /tmp/rsyncd.log

[web]
	comment = web files
	ignore errors = no
	read only = no
	write only = no
	list = false
	auth users=$RSYNCUSER
	secrets file = $RSYNCSECRET
	hosts allow = $1
	hosts deny  = *
	uid = www
	gid = www
	path = /home/wwwroot/default
EOF

#end of rsyncd.conf

	else
		echo "$RSYNC_CONF already existed"
	fi
}

slave_gen_start_files()
{
	if ! [ -f $RSYNC_CLIENT ]; then

# gen start slave file

cat > $RSYNC_CLIENT << EOF
#!/usr/bin/env bash
 
CRONTAB=\$(which crontab)
CRONCONF="/tmp/tmp_cron.conf"
 
useradd www &>/dev/null
killall rsync &>/dev/null
rm -rf /tmp/rsyncd.*
mkdir -p /home/wwwroot/default &>/dev/null
chown -R www:www /home/wwwroot/default &>/dev/null

$RSYNC --daemon --config=$RSYNC_CONF --port=$SYNC_PORT \$@
 
\$CRONTAB -l | sed '/sync_check.sh/d'> \$CRONCONF
echo "*/1 * * * * /root/sync/sync_check.sh checkslave >>/tmp/cron_slave.log" >> \$CRONCONF
\$CRONTAB \$CRONCONF
rm -rf \$CRONCONF

EOF

#end of start slave file
	fi

	chmod u+x $RSYNC_CLIENT 

	return 0
}

slave_update_sync_check()
{
	tmpfile="/tmp/tmp_slave_sync"

	cmd="/$RSYNC_CLIENT_NAME/c \	\	$RSYNC_CHECK"
	sed "$cmd" $RSYNC_CHECK >$tmpfile
	mv -f $tmpfile $RSYNC_CHECK


	chmod u+x $RSYNC_CHECK
}

# sync master 
be_slave()
{
	echo "role: slave"
	
	if [ $# -lt 1 ]; then
		echo "master server address missing"
		return 0
	fi

	if ! ping -c1 -W1 $1 &>/dev/null; then
		echo "$1 could not reachable, try again later"
		return 0
	fi

	slave_create_pass
	slave_gen_rsync_conf $*
	slave_gen_start_files $*
	slave_update_sync_check

	echo "config ready. file $RSYNC_CONF"

	return 0
}


master_cron_job()
{
	local CRONTAB=$(which crontab)
	local CRONCONF=/tmp/tmp_cron.conf

	$CRONTAB -l | sed '/sync_check.sh/d' | sed '/do_one_sync.sh/d' > $CRONCONF
	echo "*/1 * * * * $RSYNC_CHECK checkmaster >>/tmp/cron_master.log" >> $CRONCONF
	echo "*/1 * * * * $(pwd)/do_one_sync.sh >>/tmp/cron_master.log" >>$CRONCONF
	$CRONTAB $CRONCONF
	rm -rf $CRONCONF
}


#root extra args
be_master()
{
	echo "role: master"
	if [ $# -lt 1 ]; then
		echo "master sync root should supplied"
	fi

	master_cron_job

	root=$1
	master_create_pass
	sync_one $@

	$INOTIFYWAIT -mrq --timefmt '%d/%m%y %H%M' --format '%T %w%f%e' \
		-e modify,delete,create,attrib,move,delete_self $root | \
	while read files
	do
		sync_one $@
	done

	return 0
}

#args
#	--exclude-from=$exclude	# if you want to exclude
sync_one()
{
	root=$1
	shift

	for serv in $RSYNC_SERVERS; do
		$RSYNC -az --delete --progress $root --port=$SYNC_PORT $@ \
			--password-file=$RSYNCSECRET $RSYNCUSER@$serv::$RSYNCUSER 
	done
}


main()
{
	case $1 in
		beslave*)
			shift
			be_slave $*
		;;
		bemaster*)
			shift
			be_master $*
		;;

		syncnow*)
			shift
			sync_one $*
		;;

		*)
			echo "Usage $0 beslave/bemaster/syncnow"
		;;
	esac
}

main $*
exit $?
