#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import threading, queue, time


num_msg = 4


def producer(idnum, mtx, pool):

    for n in range(num_msg):
        time.sleep(idnum)
        pool.put('[producer {}], count {}'.format(idnum, n))

def customer(idnum, mtx, pool):

    while True:
        time.sleep(0.1)

        try:
            msg = pool.get(block=False)
        except queue.Empty:
            pass
        else:
            with mtx:
                print("consumer {} got {}".format(idnum, msg))
        

def _test():

    mtx = threading.Lock()

    threads = []
    pool = queue.Queue()

    for i in range(5):
        t = threading.Thread(target=customer, args=(i, mtx, pool))
        t.daemon = True
        t.start()
        threads.append(t)

    for i in range(3):
        t = threading.Thread(target=producer, args=(i, mtx, pool))
        t.daemon = True
        t.start()
        threads.append(t)

    for t in threads:
        t.join()


if __name__ == '__main__':
    _test()

    print('Main Thread Exit...')
