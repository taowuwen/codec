#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import _thread
import time


def child(tid):
    print('Hello from thread', tid)


def parent():
    i = 0

    while True:
        i += 1
        _thread.start_new_thread(child, (i,))

        if input() == 'q':
            break

parent()


class Power:
    def __init__(self, i):
        self.i = i

    def action(self):
        print (self.i ** 32)


def action(i):
    print(i ** 32)


_thread.start_new_thread(action, (2, ))
_thread.start_new_thread((lambda : action(2)), ())

obj = Power(2)

_thread.start_new_thread(obj.action, ())

time.sleep(1)
    
