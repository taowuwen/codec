#!/usr/bin/env bash

nodes_ordered_startup="
/etc/xen/hi-spider.cfg
"

nodes_startup="
/etc/xen/vm.debian.00.cfg
/etc/xen/vm.debian.01.cfg
/etc/xen/vm.debian.02.cfg
/etc/xen/vm.debian.03.cfg
/etc/xen/vm.debian.04.cfg
/etc/xen/vm.debian.05.cfg
/etc/xen/winxp.cfg
"

nodes_no_startup="
/etc/xen/vm.debian.06.cfg
/etc/xen/vm.debian.07.cfg
/etc/xen/vm.debian.08.cfg
/etc/xen/vm.debian.09.cfg
/etc/xen/vm.debian.10.cfg
/etc/xen/win10.cfg
"


./vm_config.py create

create_ordered_startup()
{
	for n in $nodes_ordered_startup
	do
		./vm_config.py  -a -e  -o -d 20 add $n
	done
}

create_startup()
{
	for n in $nodes_startup
	do
		./vm_config.py  -a -e  add $n
	done
}

create_no_startup()
{
	for n in $nodes_no_startup
	do
		./vm_config.py add $n
	done
}


create_ordered_startup
./vm_config.py list
create_startup
./vm_config.py list
create_no_startup

./vm_config.py list

