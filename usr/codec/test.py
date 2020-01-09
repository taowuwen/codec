#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from dbgview import DebugStringPrint, DebugStringConfig
import dbgview
import gzip


DebugStringPrint(f'dbgview 列表: {dir(dbgview)}')
DebugStringPrint(f'gzip 列表: {dir(gzip)}')


for item in dir(dbgview) + dir(gzip):
    DebugStringPrint(f'     {item}')
