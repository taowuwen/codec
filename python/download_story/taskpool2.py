#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import threading
import queue
import time
import enum

from urllib.parse import urlparse

task_status = enum.Enum(
    value = 'task_status',
    names = ('unknown inited running stopped paused closed done error')
)


class NotImplemented(Exception):
    pass

class Task:
    task = u'task'
    def __init__(self):
        self._ctx = None
        self._st  = task_status.inited

    @property
    def status(self):
        return self._st

    @status.setter
    def status(self, new_st):
        self._st = new_st

    @staticmethod
    def get_parser(clsname="quanben"):

        from parser import quanben

        return {
                "quanben": quanben
            }.get(clsname, quanben)

    def is_done(self):
        return self._st in [task_status.done, task_status.error, task_status.closed]

    def task_run(self):
        raise NotImplemented()

    def __str__(self):
        return "{0.__class__.__name__} ---> {0._st.name}".format(self)



class TaskHttp(Task):

    task = 'http'

    def __init__(self, url, cls):
        assert url and cls, "url and cls not null"
        assert isinstance(url, str)

        super(TaskHttp, self).__init__()
        self._url = url
        self._cls = cls()

    def __str__(self):
        return super(TaskHttp, self).__str__() +\
                "[{:10}: {url}]".format(threading.current_thread().name, url=self._url)

    def task_run(self):

        self.status = task_status.running

        try:
            self._ctx = self._cls.http_get(self._url)

        except Exception as e:
            #import traceback
            #traceback.print_tb(e.__traceback__)
            print("------Warnning {} failed for '{}'-----".format(self, e))
            self.status = task_status.error

        else:
            assert self._ctx, "_ctx should never be None here"
            self.status = task_status.done


class TaskMenuDownload(TaskHttp):

    task = 'menu'

    def __init__(self, url, cls="quanben"):
        super(TaskMenuDownload, self).__init__(url, self.get_parser(cls).MenuDownload)
        self._cur = None

    def __iter__(self):
        self._cur = iter(self._ctx)
        return self

    def __next__(self):
        return next(self._cur)

    def keys(self):
        return self._cur.keys()

    def values(self):
        return self._cur.values()

    def items(self):
        return self._cur.items()

    def __getitem__(self, key):
        return self._ctx[key]


class TaskPageDownload(TaskHttp):

    task = 'page'

    def __init__(self, url, cls="quanben"):
        super(TaskPageDownload, self).__init__(url, self.get_parser(cls).PageDownload)

    @property
    def content(self):
        assert self.status is task_status.done and self._ctx, "status should be done {}".format(self)
        return self._ctx['content']

    @property
    def title(self):
        assert self.status is task_status.done and self._ctx, "status should be done {}".format(self)
        return self._ctx['title']

    @property
    def menu(self):
        assert self.status is task_status.done and self._ctx, "status should be done {}".format(self)
        return self._ctx['menu']

    @property
    def current(self):
        return self._url


# add_tasks 
# start()
#   start num threads
#   wait for threads stop
#
# child threads
#   fetch one tasks, and run


class taskpool2:

    def __init__(self, num=1, output=sys.stdout):

        self._wait = queue.Queue()

        self._num_child = num
        self._workers = [] 
        self._work_mtx = threading.Lock()
        self._working = 0
        self._running = []

        self._finished = queue.Queue()

        self._output = output
        self._last_task = None

        self._st = task_status.inited
        self._retry = 10

        self._failed = 0
        self._succeed = 0
        self._total = 0

    def consume_default(self, task):
        print("not support for {} {}".format(task, task.task))

    def consume_page(self, task):
        assert task.task is "page", "task should be page"
        assert task.is_done(), "task should be done"

        if task.status is task_status.done:
            self._output.write("\n" + task.title + "\n" + task.content)

            if not self._last_task:
                self.add_task(TaskMenuDownload(task.menu))

            self._last_task = urlparse(task.current).path
        else:
            self._output.write("\n------ failed {} -----\n".format(task))


    def consume_menu(self, task):
        assert task.task is "menu", "task should be page"

        if self._last_task:
            skip = 1
            for item in task:

                if skip:
                    if self._last_task == urlparse(task[item]).path:
                        skip = 0

                    continue
                else:
                    self.add_task(TaskPageDownload(urlparse(task[item]).path))
        else:
            for item in task:
                self.add_task(TaskPageDownload(urlparse(task[item]).path))


    def get_consumer(self, task):
        return {"page": self.consume_page,
                "menu": self.consume_menu
               }.get(task, self.consume_default)


    def run(self):

        self._st = task_status.running

        # start workers
        for n in range(self._num_child):
            th = threading.Thread(target=self._child_main)
            th.start()
            self._workers.append(th)

        time.sleep(0.5)

        # run as customer
        while True:

            try:
                # get finished task 
                task = self._finished.get(block=False)

            except queue.Empty:

                if not self._working and self._finished.empty() and self._wait.empty():
                    self._st = task_status.stopped

                    for th in self._workers:
                        th.join()

                    print("All done: total {} SUCCEED(~_~): {} FAILED(~@~): {}".format(self._total, self._succeed, self._failed))

                    sys.exit(0)

                time.sleep(0.5)

            else:
                consumer = self.get_consumer(task.task)
                consumer(task)

    def _task_run(self, task, retry):
        # do run task 
        while retry:
            if self._st is task_status.running:
                task.task_run()
            else:
                break

            if task.status is task_status.done:
                break

            retry -= 1

        print(task)

    def _child_main(self):

        while self._st is task_status.running:

            try:
                task = self._wait.get(block=False)
                print("{} left: {}".format(task, self._wait.qsize()))

            except queue.Empty:
                time.sleep(0.5)

            else:
                # make sure task is ordered
                with self._work_mtx:
                    self._working += 1
                    self._running.append(task)

                self._task_run(task, self._retry)

                with self._work_mtx:
                    if task is self._running[0]:
                        assert task.is_done(), "task not done" 

                        while self._running:
                            tsk = self._running[0]
                            if not tsk.is_done():
                                break

                            del self._running[0]

                            # add finished task
                            self._finished.put(tsk)

                    if task.status is task_status.done:
                        self._succeed += 1
                    else:
                        self._failed  += 1

                    self._working -= 1


    def add_task(self, task):
        self._total += 1
        print("add {}".format(task))
        self._wait.put(task)



if __name__ == '__main__':

    url = 'http://www.quanben5.com/n/jiuzhuanhunchunjue/27339.html'

    with open("/tmp/tmp.txt", "a+") as fp:
        tp = taskpool2(num=10, output=fp)
        tp.add_task(TaskPageDownload(url))
        tp.run()
