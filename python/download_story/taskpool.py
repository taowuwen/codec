#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import threading
import queue
import time

import tasks

class TaskPool:
    def __init__(self, num=1, fl="/tmp/tmp.txt"):
        self._max_child = num
        self._task_pool = queue.Queue()
        self._mtx = threading.Lock()
        self._running = tasks.TaskFileWrite(fl)
        self._current = 0
        self._st = tasks.st_init

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

        self._head.status = tasks.st_done
        
        while True:

            if self._current >= self._max_child:
                time.sleep(0.5)
                continue

            try:
                task = self._task_pool.get(block=False)
            except queue.Empty:
                if self._current == 0:
                    print("No more tasks, and no running tasks, do exit...")
                    # here flush TaskFileWrite maybe
                    return

                time.sleep(0.5)
            else:
                self._add_running_task(task)


    def _task_run(self, task, retry):

        task.status = tasks.st_running
        ret = 1
        print("***START*** {} current: {}".format(task, self._current))

        while retry:
            retry -= 1
            try:
                ret = task.task_run()
            except Exception as e:
                print(e)

            else:
                if ret == 0:
                    break

        task.status = tasks.st_done

        if ret == 1:
            print("---WARNING--- failed on: {}".format(task))
            task._ctx = "----------failed On:  {}----\n".format(task)

        with self._mtx:

            if task._prev is self._head:

                while task._next is not self._head and \
                        task._next.status == tasks.st_done:
                    task.task_merge_next(task._next)

                task._prev.task_merge_next(task)

            self._current -= 1
            print("***DONE*** {} current: {}".format(task, self._current))

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
    from tasks import TaskPageDownload

    tp = TaskPool(num=20)

    tp.task_append(TaskPageDownload("/n/jiuzhuanhunchunjue/27535.html"))
    tp.start()
