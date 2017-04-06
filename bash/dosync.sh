#/usr/bin/env sh

mv sync.tgz sync_backup.tgz

tar cvzf sync.tgz sync


do_sync_server()
{
	server=$1

	ssh root@$1 'rm -rf sync_backup; mv -f sync sync_backup'
	ssh root@$1 'mv -f sync.tgz sync_backup.tgz'
	scp sync.tgz root@$1:/root
	ssh root@$1 'tar xvzf sync.tgz'
}

do_sync_server 172.16.127.10
do_sync_server 172.16.127.20
do_sync_server 172.16.127.30
