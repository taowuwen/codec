#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import threading
import queue
import time

import tasks



# add_tasks 
# start()
#   start num threads
#   wait for threads stop
#
# child threads
#   fetch one tasks, and run


class taskpool2:

    def __init__(self, num=1, filename='/tmp/tmp.txt'):

        self._max_child = num
        self._task_pool = queue.Queue()
        self._mtx = threading.Lock()
        self._running = tasks.TaskFileWrite(fl)
        self._current = 0
        self._st = tasks.st_init
