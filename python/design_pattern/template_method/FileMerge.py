#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os

class MergeFile:

    def __init__(self, *args):

        self._input  = None
        self._output = None
        print(*args, len(args))

        if len(args) >= 2:
            self._input = list(args[:-1])
            self._output = args[-1]

            self.merge()


    def merge(self):

        if not self._input or not self._output:
            return None

        print("Input files: {}".format(self._input))
        print("Output file: {}".format(self._output))

        for fl in self._input:
            with open(self._output, "a") as fout:
                fout.write(self.handle(fl))

    def handle(self, fl):
        txt = ""
        with open(fl, "r") as fin:
            txt = fin.read()

        return txt

class UppercaseFile(MergeFile):

    def handle(self, fl):
        return super().handle(fl).upper()

class SearchFile(MergeFile):

    def __init__(self, *args):

        self._key = set()
        self._build_keys(args[0])

        super().__init__(*args[1:])

    def _build_keys(self, fl):

        txt = ""

        with open(fl, "r") as fin:
            txt = fin.read()

        import re
        import string

        _txt = re.split('[' + string.punctuation + string.whitespace + ']', txt)

        [ self._key.add(a) for a in _txt if a ]


    def handle(self, fl):
        txt = ""
        with open(fl, "r") as fin:

            for ln, l  in enumerate(fin):

                for k in self._key:
                    if k in l:
                        txt = txt + "{} found in file {}, ln {}\n".format(k, fl, ln)
        return txt



if __name__ == '__main__':
    print("testing merge file {}".format(sys.argv))

    #mf = MergeFile(*sys.argv[1:])
    #mf.merge()

    #UppercaseFile(*sys.argv[1:])
    SearchFile(*sys.argv[1:])
