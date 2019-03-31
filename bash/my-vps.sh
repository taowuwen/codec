#!/usr/bin/env bash

_start_vps()
{
	local port=$1
	local pass="$2"

	ss-server -p $port -k "$pass" -m 'aes-256-cfb' -u >/tmp/shadowsocks.$port.log 2>&1 &
}

USERS="
9000:an_cuit_123456789
9001:flj_cuit_123456
9002:wrq_cuit_123456
9003:wwy123456
"

for user in $USERS
do
	port="`echo $user | cut -d: -f1`"
	pass="`echo $user | cut -d: -f2`"

	[ -n "$port" -a -n "$pass" ] && _start_vps "$port" "$pass"
done
