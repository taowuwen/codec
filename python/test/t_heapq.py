#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import math
from io import StringIO
import random
import heapq
import time

def show_tree(tree, total_width=36, fill=' '):

    output = StringIO()
    last_row = -1

    for i, n in enumerate(tree):

        if i:
            row = int(math.floor(math.log(i + 1, 2)))
        else:
            row = 0

        if row != last_row:
            output.write('\n')

        columns = 2 ** row

        col_width = int(math.floor(total_width/columns))
        output.write(str(n).center(col_width, fill))

        last_row = row

    print(output.getvalue())
    print('-' * total_width)
    print()




if __name__ == '__main__':
    data = [19, 9, 4, 10, 11]

    heap = []

    print('Data: ', data)
    print()

    for n in data:
        print("{: <3}".format(n))

        heapq.heappush(heap, n)
        show_tree(heap)
        print(heap)

    data1 = data[:]

    heapq.heapify(data1)
    show_tree(data1)
    print(data1)

    for n in range(2):
        s = heapq.heappop(data1)
        print('POPed out: ', s)
        show_tree(data1)
        print(data1)


    for n in [1, 13]:
        s = heapq.heapreplace(data1, n)
        print('replaced out: ', s)
        show_tree(data1)
        print(data1)


    print('nlargest: ', heapq.nlargest(3, data))
    print('from sort ', list(reversed(sorted(data)[-3:])))
    print('nsmallest: ', heapq.nsmallest(3, data))
    print('sort ', sorted(data)[:3])

    print('testing sort....')

    random.seed(int(time.time()))

    data = []

    for n in range(4):

        n = list(random.sample(range(1, 100), 5))
        n.sort()
        data.append(n)

    for i, l in enumerate(data):
        print("{:<3} {}".format(i, l))

    data_merge = heapq.merge(*data)
    for i in data_merge:
        print(i, end=' ')

