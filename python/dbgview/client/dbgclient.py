#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from dbgview import DebugStringPrint, DebugStringConfig
import time


if __name__ == '__main__':
    DebugStringPrint("hello, world")
    DebugStringPrint("tww, world")
    DebugStringPrint("aaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")

    DebugStringConfig(enable_thread=1, timeout=1)
    DebugStringPrint("hello, world")
    DebugStringPrint("tww, world")
    DebugStringPrint("aaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("tww love xixi, world")

    for x in range(10000):
        DebugStringPrint(f"tww love xixi with {x} times")
