#!/usr/bin/env python3

from array import array
from copy import copy

class InvalidDimensionArray(Exception): pass

class DSArray:

    def __init__(self, _sz = 32):
        self._size = _sz
        self._len  = 0
        self._data = array('h', [0 for x in range(self._size)])

    def _offset(self, idx):
        if not 0 <= idx < self._size:
            raise IndexError('Out of Index')
        return idx

    def __getitem__(self, idx):
        return self._data[self._offset(idx)]

    def __setitem__(self, idx, item):
        self._data[self._offset(idx)] = item

    def __str__(self):
        return " ".join(str(self._data[a]) for a in range(self._size) )


class ListBasedArray:
    def __init__(self, _size):
        self._pos = 0
        self._size = _size
        self.arry = DSArray(self._size)

    def insert(self, pos, item):
        pass

    def append(self, item):
        pass

    def pop(self, item):
        pass

    def remove(self, idx):
        pass

    def find(self, item):
        pass

    def index(self, item):
        pass

class QueueBasedArray:
    def __init__(self, _size):
        self._pos = 0
        self._size = _size
        self._ary = DSArray(_size)

    def __str__(self):
        res=[str(self._ary[a]) for a in range(self._pos)]
        return ", ".join(res)

    @property
    def empty(self):
        return self._pos == 0

    @property
    def full(self):
        return self._pos == self._size

    def put(self, item):
        if self.full:
            return -1

        self._ary[self._pos] = item
        self._pos += 1

        return 0

    def get(self):
        if self.empty:
            return None

        res = self._ary[0]

        for x in range(self._pos-1):
            self._ary[x] = self._ary[x+1]

        self._pos -= 1

        return res

    def __len__(self):
        return self._pos

    def size(self):
        return self._size

class CircleQueueBaseArray:
    def __init__(self, _size):
        self._head = 0
        self._tail = -1
        self._size = 0
        self._maxsize = _size
        self._ary = DSArray(_size)

    def __str__(self):

        if self.empty:
            return "[]"

        if self._tail > self._head:
            return f'[{", ".join([str(self._ary[a]) for a in range(self._head,self._tail +1)])}]'
        else:
            first_part=[str(self._ary[a]) for a in range(self._head, self._maxsize)]
            second_part=[str(self._ary[a]) for a in range(self._tail)]

            return f'{", ".join(first_part)}, {", ".join(second_part)}'


    def put(self, item):
        if self.full:
            return None

        self._tail = (self._tail + 1) % self._maxsize
        self._ary[self._tail] = item
        self._size += 1

    def get(self):
        if self.empty:
            return None

        res = self._ary[self._head]
        self._head = (self._head + 1) % self._maxsize
        self._size -= 1
        return res

    @property
    def full(self):
        return self._size == self._maxsize

    @property
    def empty(self):
        return self._size == 0

    def __len__(self):
        return self.size()

    def size(self):
        return self._size

class StackBasedArray:
    def __init__(self, _size):
        self._pos = 0
        self._size = _size
        self._ary = DSArray(self._size)

    def pop(self):
        if self.empty():
            return None

        self._pos -= 1
        return self._ary[self._pos]

    def push(self, item):
        if self.full():
            return -1

        self._ary[self._pos] = item
        self._pos += 1
        return 0

    def full(self):
        return self._pos == self._size

    def empty(self):
        return self._pos == 0

    @property
    def length(self):
        return self._pos

    def __str__(self):
        return f'[{", ".join(str(self._ary[a]) for a in range(self._pos))}]'



class MultiDimensionalArray:
    def __init__(self, *dimensions):
        self._dimensions = [ d for d in dimensions if d > 0]
        self._ND         = len(dimensions)

        if len(self._dimensions) != self._ND:
            raise InvalidDimensionArray('Invalid Arguments')

        self._ary = DSArray(self.multiple_all(self._dimensions))

        self._factors = [1 for d in range(self._ND)]
        idx = self._ND -1

        while idx > 0:
            self._factors[idx - 1] = self._factors[idx] * self._dimensions[idx]
            idx -= 1

        print(self._factors)

    def multiple_all(self, lst):
        res = 1

        for item in lst:
            res *= item

        return res

    def _offset(self, key):
        _pos = 0
        if not isinstance(key, tuple) or len(key) != self._ND:
            raise InvalidDimensionArray(f'this is a {self._ND} demension array, not {len(key)}')

        for idx in range(self._ND):
            if key[idx] < 0 or key[idx] >= self._dimensions[idx]:
                raise IndexError('Index Error for {key}, pos {idx}')

        return sum([key[idx] * self._factors[idx] for idx in range(self._ND)])

    def __getitem__(self, key):
        return self._ary[self._offset(key)]

    def __setitem__(self, key, val):
        self._ary[self._offset(key)] = val

    def __str__(self):
        return str(self._ary)

def main():

    arr = DSArray(10)
    arr[1] = 12
    print(arr)

    stack = StackBasedArray(10)
    stack.push(1)
    stack.push(2)
    stack.push(3)
    print(stack)
    stack.pop()
    print(stack)
    stack.pop()
    print(stack)

    stack.pop()
    print(stack)

    mary = MultiDimensionalArray(2,3,4)
    mary[1,2,3] = 23
    mary[0,1,2] = 13
    mary[0,0,0] = 100
    mary[1,0,0] = 12

    print(mary)
    print(mary[1,2,3])

    q_circle = CircleQueueBaseArray(10)
    q_circle.put(1)
    q_circle.put(2)
    q_circle.put(3)
    q_circle.put(4)
    q_circle.put(5)

    print(q_circle)
    q_circle.put(1)
    q_circle.put(2)
    q_circle.put(3)
    q_circle.put(4)
    q_circle.put(5)
    print(q_circle)
    print(len(q_circle), q_circle.size())

    q_circle.put(5)
    print(q_circle)

    print(q_circle.get())
    print(q_circle.get())
    print(q_circle.get())
    print(q_circle.get())
    print(q_circle.get())

    q_circle.put(4)
    q_circle.put(5)
    print(q_circle)
    print(len(q_circle), q_circle.size())

    q_circle.put(5)
    print(q_circle)

    queue = QueueBasedArray(10)
    queue.put(1)
    queue.put(2)
    queue.put(4)
    queue.put(5)
    queue.put(3)
    queue.put(6)
    print(queue)

    queue.put(7)
    queue.put(8)
    queue.put(9)
    queue.put(10)
    print(queue)
    queue.put(10)
    print(queue)
    queue.put(11)
    print(queue)
    queue.put(12)
    print(queue)

    print(queue.get())
    print(queue.get())
    print(queue.get())
    print(queue.get())
    print(queue.get())
    print(queue.get())
    print(queue)
    queue.put(7)
    queue.put(8)
    queue.put(9)
    queue.put(10)
    print(queue)
    queue.put(10)
    print(queue)
    queue.put(11)
    print(queue)
    queue.put(12)
    print(queue)


if __name__ == '__main__':
    main()
