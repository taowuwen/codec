#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import shlex
import subprocess


def do_test():

    cmd = "ls -l"
    print(subprocess.Popen(shlex.split(cmd)))


if __name__ == '__main__':
    do_test()
