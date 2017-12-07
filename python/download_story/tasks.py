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
        print("Download Page from {}".format(self._url))
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
        if not self._url_path:
            self._pool.task_append(TaskMenuDownload(task._data["menu"]))

        self._url_path = urlparse(task._url).path

        super(TaskFileWrite, self).task_merge_next(task)

        print("write downloaded pages into file")
        with open(self._fl, "a+") as fl:
            fl.write(self._ctx)

        self._ctx = ""

    def __str__(self):
        return "{0.__name__}: {0._fl}".format(self)


class TaskPool:
    def __init__(self, num=1, fl="/tmp/tmp.txt"):
        self._max_child = num
        self._task_pool = queue.Queue()
        self._mtx = threading.Lock()
        self._running = TaskFileWrite(fl)
        self._current = 0
        self._st = st_init

        '''
            make running be a double linked list
        '''
        head = self._running
        head._prev = self._running
        head._next = self._running

        self._head = head
        self._head._pool = self

        self._retry = 3


    def start(self):

        self._head.status = st_done
        
        while True:

            if self._current >= self._max_child or self._task_pool.empty():
                time.sleep(0.5)
                continue

            try:
                task = self._task_pool.get(block=False)
            except queue.Empty:
                if self._current == 0:
                    print("do exit here")
                    # here flush TaskFileWrite maybe
                    return
                pass
            else:
                self._add_running_task(task)


    def _task_run(self, task, retry):

        print("task for : {} started, current {}".format(task, self._current))

        task.status = st_running
        ret = 1

        while retry:
            retry -= 1
            ret = task.task_run()
            if ret == 0:
                break


        task.status = st_done

        if ret == 1:
            task._ctx = "----------failed On:  {}----\n".format(task)

        print("task for : {} done, status: {}".format(task, ret))

        with self._mtx:

            if task._prev is self._head:

                while task._next is not self._head and \
                        task._next.status == st_done:
                    print("task merge next task: {}".format(task))
                    task.task_merge_next(task._next)

                print("task merge SELF: {}, current {}".format(task, self._current))
                task._prev.task_merge_next(task)

            self._current -= 1

        print("thread exit ... current: {}".format(self._current))

    def _add_running_task(self, task):
        '''
        add running task into tail
        '''
        with self._mtx:
            self._head.task_insert_tail(task)

            self._current += 1
            task._pool = self

        threading.Thread(target=self._task_run, args=(task, self._retry)).start()

    def task_append(self, task):
        print("add task {}".format(task))
        self._task_pool.put(task)


    @property
    def status(self):
        return self._status


if __name__ == '__main__':
    tp = TaskPool(num=20)

    tp.task_append(TaskPageDownload("/n/jiuzhuanhunchunjue/27535.html"))
    tp.start()
