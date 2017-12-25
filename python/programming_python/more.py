#!/usr/bin/env python3
# -*- coding: utf-8 -*-



def more(ctx, ln):

    lns = ctx.splitlines()

    while lns:

        chunk = lns[:ln]
        lns   = lns[ln:]

        for l in chunk:
            print(l)

        if lns and input('More?') not in ['y', 'Y']:
            break



if __name__ == '__main__':

    import os
    import sys
    more(open(sys.argv[1]).read(), 10)
