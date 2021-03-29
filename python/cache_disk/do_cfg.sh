#!/usr/bin/env bash

set -e

disk_root=${HOME}/cache_disk

[ -d $disk_root ] || mkdir -p $disk_root

fuse_mount()
{
    [ -d $disk_root/fuse ] || mkdir -p $disk_root/fuse
    ./fgwctl fuse start $disk_root/fuse
}

disk_mount()
{
    # 1G == 1073741824

    disks="0 1 2 3 4"
    for disk in $disks
    do
        [ -d $disk_root/mnt/$disk ] || mkdir -p $disk_root/mnt/$disk
        ./fgwctl disk start $disk_root/mnt/$disk 1073741824
    done
}

mem_mount()
{
    mem_root=/tmp/mem
    [ -d $mem_root ] || mkdir -p $mem_root
    ./fgwctl mem start $mem_root 1073741824
}

ssd_mount()
{
    ssds="0 1"

    for disk in $ssds
    do
        [ -d $disk_root/ssd/$disk ] || mkdir -p $disk_root/ssd/$disk
        ./fgwctl ssd start $disk_root/ssd/$disk 1073741824
    done

}

do_start()
{
    echo "do start"
    fuse_mount
#    disk_mount
    mem_mount
    ssd_mount
}

do_stop()
{
    echo "do stop"
    # unprepare memory
    # unprepare disks
    # unprepare ssd
    # unprepare fuse
    # umount fuse
}

do_restart()
{
    do_stop
    do_start
}

tgt=${1:-start}

type do_$tgt &>/dev/null || {
    echo "unkown method '$tgt' (current support start/stop/restart)"
    exit 1
}

do_$tgt $*
