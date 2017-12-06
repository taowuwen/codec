#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import _thread
import time
import threading


def child(tid):
    print('Hello from thread', tid)


def test_1():
    i = 0

    while True:
        i += 1
        _thread.start_new_thread(child, (i,))

        if input() == 'q':
            break


class Power:
    def __init__(self, i):
        self.i = i

    def action(self):
        print (self.i ** 32)


def action(i):
    print(i ** 32)


def test_2():
    _thread.start_new_thread(action, (2, ))
    _thread.start_new_thread((lambda : action(2)), ())

    obj = Power(2)

    _thread.start_new_thread(obj.action, ())

    time.sleep(1)


def test_3():
    def counter(myid, count):
        for i in range(count):
            time.sleep(1)
            print('[{}] => {}'.format(myid, i))

    for i in range(5):
        _thread.start_new_thread(counter, (i, 5))

    time.sleep(6)


def test_mtx():

    mtx = _thread.allocate_lock()

    def counter(myid, count):
        for i in range(count):
            time.sleep(1)
            mtx.acquire()
            print('[{}] => {}'.format(myid, i))
            mtx.release()

    def counter_v1(myid, count):
        for i in range(count):
            time.sleep(1)
            with mtx:
                print('[{}] => {}'.format(myid, i))

    for i in range(5):
        _thread.start_new_thread(counter, (i, 5))

    for i in range(5, 10):
        _thread.start_new_thread(counter_v1, (i, 5))

    time.sleep(6)


def test_threading():

    class MyThread(threading.Thread):
        def __init__(self, myid, count, mtx):
            self.id = myid
            self.count = count
            self.mtx = mtx
            super(MyThread, self).__init__()

        def run(self):
            for i in range(self.count):
                with self.mtx:
                    print('[{}] => {}'.format(self.id, i))

    mtx = threading.Lock()

    threads = []

    for i in range(10):
        thread = MyThread(i, 100, mtx)
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()

def test_add_v1():

    count = 0

    def adder(mtx):
        nonlocal count
        time.sleep(0.5)
        with mtx:
            count = count + 1
        time.sleep(0.5)
        with mtx:
            count = count + 1

    threads = []

    mtx = threading.Lock()

    for i in range(100):
        thread = threading.Thread(target=adder, args=(mtx,))
        thread.start()
        threads.append(thread)

    for t in threads:
        t.join()

    print("count = {}".format(count))



def _main():
#    test_1()
#    test_2()
#    test_3()
#    test_mtx()
#    test_threading()
    test_add_v1()
    print('Main thread exiting....')


if __name__ == '__main__':
    print("hello, test thread")
    _main()
