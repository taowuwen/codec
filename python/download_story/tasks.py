#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import threading
import queue
import time

from parser import quanben
from urllib.parse import urlparse


def _get_parser(clsname="quanben"):
    return {
        "quanben": quanben
    }.get(clsname, quanben)


(st_unkowned, st_init, st_done, st_running, st_closed) = range(5)


class NotSupportted(Exception): pass


class Task:
    def __init__(self):
        self._prev = None
        self._next = None
        self._st   = st_unkowned
        self._pool = None
        self._ctx  = ""
        self._data = None

    @property
    def status(self):
        return self._st

    @status.setter
    def status(self, st):
        self._st = st

    def task_run(self):
        print("should implement by child")

    def task_insert_tail(self, task):
        '''
        self == head
        '''
        _head = self
        _head._prev._next = task
        task._prev = _head._prev
        task._next = _head
        _head._prev = task


    def task_merge_next(self, _n):
        '''
            _n: next node
        '''
        assert self.status == st_done, "status should be done"
        assert _n and _n.status == st_done

        self._ctx += _n._ctx

        _n._next._prev = self
        self._next = _n._next

        _n._prev = None
        _n._next = None
        _n._pool = None
        _n._ctx  = None
        _n.status = st_closed


class TaskHttp(Task):
    def __init__(self, url, cls):
        super(Task, self).__init__()
        self._url = url
        self._cls = cls()

    def task_run(self):
        print("should reality by child {}".format(self))
        raise NotSupportted()

    def __str__(self):
        return "{0._cls.__class__.__name__}: {0._url}".format(self)

    def task_merge_next(self, _n):
        self._url = _n._url
        super(TaskHttp, self).task_merge_next(_n)

class TaskMenuDownload(TaskHttp):
    def __init__(self, url, cls="quanben"):
        super(TaskMenuDownload, self).__init__(url, _get_parser(cls).MenuDownload)

    def task_run(self):
        print("Download Menu from {}".format(self._url))
        menu = self._cls.http_get(self._url)
        self._ctx = ""
        if menu:
            self._data = menu

            if self._pool._head._url_path:
                # remove unused items
                keys = list(menu.keys())
                for key in keys:

                    path = urlparse(menu[key]).path
                    del menu[key]

                    if path == self._pool._head._url_path:
                        break

            # add rest targets
            for key in menu.keys():
                self._pool.task_append(TaskPageDownload(menu[key]))

            return 0

        return 1


class TaskPageDownload(TaskHttp):
    def __init__(self, url, cls="quanben"):
        super(TaskPageDownload, self).__init__(url, _get_parser(cls).PageDownload)

    def task_run(self):
        page = self._cls.http_get(self._url)
        if page:
            self._data = page
            self._ctx = "\n{title}\n{content}".format(
                            title=page["title"],
                            content=page["content"]
                        )
            return 0

        return 1


class TaskFileWrite(Task):
    def __init__(self, fl):
        super(TaskFileWrite, self).__init__()
        self._fl = fl
        self._url_path = None

    def task_run(self):
        pass

    def task_merge_next(self, task):

        if not self._url_path and hasattr(task, '_data'):
            self._pool.task_append(TaskMenuDownload(task._data["menu"]))

        self._url_path = urlparse(task._url).path

        super(TaskFileWrite, self).task_merge_next(task)

        with open(self._fl, "a+") as fl:
            fl.write(self._ctx)

        self._ctx = ""

    def __str__(self):
        return "{0.__name__}: {0._fl}".format(self)


