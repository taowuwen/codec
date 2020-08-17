#!/usr/bin/env python3

import sys
import os


def main():
    '''
        [ SERVER ] <---put-- dbgserver.py <--- debugstringprint
            |
            |
            +---------------+---------------+---------------+
            |               |               |               |
            |               |               |               |
            V               V               V               V
          [client1]     [client 2]      [client ...]    [ client n ]

    1. client register to server
    2. server keeped all clients

    server recved msg from msg queue
    server dispatch all the msgs to clients in base64 encoded strings, maybe with thread pools maybe much better

    UDP / TCP ?
    blocked / non-block ?

    thread exit or not ?

    json?
    base64?
    streams...?
    '''

    print("be a server deamon")


if __name__ == '__main__':
    print("be server...")

    sys.exit(not main())


