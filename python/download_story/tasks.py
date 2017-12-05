#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import threading
import queue
import time

class Task:
    def __init__(self):
        self._prev = None
        self._next = None
        self._st   = 0
        self._pool = None

    def task_run(self):
        print("should implement by child")


class TaskMenuDownload(Task):
    def __init__(self, url, cls):
        super(Task, self).__init__()
        self._url = url
        self._cls = cls()

    def task_run(self):
        print("Download Menu from {}".format(self._url))
        menu = self._cls.http_get(self._url)


class TaskPageDownload(task):
    def __init__(self, url, cls):
        super(Task, self).__init__()
        self._url = url
        self._cls = cls()

    def task_run(self):
        print("Download Page from {}".format(self._url))
        page = self._cls.http_get(self._url)



class TaskPool:
    def __init__(self, num_child=1):
        self._max_child = num_child
        self._task_pool = qeueu.Queue()
        self._mtx = threading.lock()
        self._running = None
        self._current = 0
        self._status = 0

    def task_run(self, task):
        self._status = 1

        if len(self._running) >= self._max_child:
            self._task_pool.put(task)

    def start(self):
        while len(self._task_pool) > 0:


    @property
    def status(self):
        return self._status



