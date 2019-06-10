#!/usr/bin/env bash

xl list | 
grep -v -e "Name\ *ID" -e "Domain-0" |
awk '{ print $1}' |
while read vm
do
	echo "shutdowning virtual machine: $vm..."
	xl shutdown $vm 2>/dev/null || xl destroy $vm
done


