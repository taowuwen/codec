#!/usr/bin/env bash

export FUSE_LIBRARY_PATH=${HOME}/usr/lib/libfuse3.so.3.6.2

ls -l $FUSE_LIBRARY_PATH

python3 ./memory.py ${HOME}/tmp
