#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from dbgview import DebugStringPrint, DebugStringConfig
import time


def test_10000():
    for x in range(10000):
        DebugStringPrint(f"tww love xixi with {x} times")


def test_10():
    for x in range(10):
        DebugStringPrint("show me in blue, world")
        DebugStringPrint("show me in red, world")
        DebugStringPrint("show me in yellow, world")
        DebugStringPrint("SHOW ME IN BLUE, WORLD")
        DebugStringPrint("SHOW ME IN RED, WORLD")
        DebugStringPrint("SHOW ME IN YELLOW, WORLD")
        DebugStringPrint("---------------blue-----------------")
        DebugStringPrint("---------------red-----------------")
        DebugStringPrint("---------------yellow-----------------")
        DebugStringPrint("---------------BLUE-----------------")
        DebugStringPrint("---------------mytarget-----------------")
        DebugStringPrint("---------------this is my target-----------------")
        DebugStringPrint("---------------this is mytarget-----------------")


def main():
    DebugStringPrint("hello, world")
    DebugStringPrint("tww, world")
    DebugStringPrint("aaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")

    DebugStringPrint("hello, world")
    DebugStringPrint("tww, world")
    DebugStringPrint("aaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("aaaaa, world")
    DebugStringPrint("tww love xixi, world")


if __name__ == '__main__':
    test_10()
    #test_10000()
