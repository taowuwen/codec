#!/usr/bin/env sh


if [ $# -lt 1 ]; then
	echo "usage: $0 master_server_address"
	exit 0
fi

./sync.sh beslave $@
