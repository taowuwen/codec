#!/usr/bin/env python3
# -*- coding: utf-8 -*_

from dbgview import DebugStringPrint, DebugStringConfig
import os
import sys
import selectors

def main(fp = None):
    DebugStringConfig(enable_thread=1, timeout=1)

    for ln in fp:
        try:
            print(ln.decode().rstrip())
            DebugStringPrint(ln.decode().rstrip())
        except Exception as e:
            pass


if __name__ == '__main__':

    if len(sys.argv) <= 1:
        sys.exit(not main(sys.stdin.buffer))

    with open(sys.argv[1], 'rb') as fp:
        sys.exit(not main(fp))

