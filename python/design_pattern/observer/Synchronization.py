#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import threading

class Synchronization:
    def __init__(self):
        self.mutex = threading.RLock()


def synchronized(method):

    print("do synchronized for {}...".format(method.__name__))
    def f(*args):

        self = args[0]
        self.mutex.acquire()

        try:
            return method(*args)

        finally:
            self.mutex.release()

    return f


def synchronize(klass, names=None):

    if names and isinstance(names, str):
        names = names.split()

    for (name, val) in klass.__dict__.items():

        if callable(val) and not name.startswith('__') and (not names or name in names):
            setattr(klass, name, synchronized(val))



