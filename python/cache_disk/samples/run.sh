#!/usr/bin/env bash

root=$(dirname `pwd`)

export PYTHONPATH=$root/fgw:$PYTHONPATH

run_memory()
{
    export FUSE_LIBRARY_PATH=${HOME}/usr/lib/libfuse3.so.3.6.2
    ls -l $FUSE_LIBRARY_PATH
    python3 ./memory.py ${HOME}/tmp
}

cmd=run_$1

if type $cmd 2>/dev/null; then
    shift
    $cmd $*
else
    [ -f $1'.py' ]  || {
        echo "$1.py not exist"
        exit 1
    }

    python $1.py $*
fi
