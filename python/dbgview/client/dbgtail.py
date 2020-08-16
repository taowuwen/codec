#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from dbgview import DebugStringPrint, DebugStringConfig
import os
import sys
import selectors
import inotify.adapters


class FileReader:

    def __init__(self, fl):
        self._fl = fl



def test_file_reader(fl):

    with open(fl, 'rb') as fp:
        pass




def main():
    i = inotify.adapters.Inotify()

    i.add_watch('/tmp/test')

    for evt in i.event_gen(yield_nones = False):
        (first, type_names, path, filename) = evt 
        print(evt)
        DebugStringPrint(evt)
        #DebugStringPrint(f'first: {first}, type: {type_names}, path, {path}, filename: {filename}')

if __name__ == '__main__':
    main()
