#!/usr/bin/env bash


ipset_version(){
	ipset --list 2>/dev/null 1>&2 || return 1

	while `iptables -S | grep -q "invalid_user"`
	do
		iptables -D INPUT -m set --match-set invalid_user src -j DROP
	done

	ipset -F invalid_user 2>/dev/null && ipset destroy invalid_user
	ipset create invalid_user hash:ip

	cat invaliduser.txt | sort | uniq |
	while read ip
	do
		[ -n "$ip" ] && ipset -A invalid_user $ip
	done

	iptables -I INPUT -m set --match-set invalid_user src -j DROP
}

iptables_version(){

	iptables -D INPUT -j INVALID_USER 2>/dev/null
	iptables -F INVALID_USER 2>/dev/null && iptables -X INVALID_USER

	iptables -N INVALID_USER && {
		iptables -I INPUT -j INVALID_USER

		cat invaliduser.txt |
		while read ip
		do
			[ -n "$ip" ] && iptables -A INVALID_USER -s $ip -j DROP
		done
	}
}


ipset_version || iptables_version
