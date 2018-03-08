#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class LinkedList:

    class Element:

        def __init__(self, list, data, next):

            self._list = list
            self._data = data
            self._next = next

        @property
        def data(self):
            return self._data

        @property
        def next(self):
            return self._next

        def insert_after(self, item):

            self._next = LinkedList.Element(self._list, item, self._next)

            if self._list._tail is self:
                self._list._tail = self._next

        def insert_before(self, item):

            _prev = LinkedList.Element(self._list, item, self)

            if self._list._head is self:
                self._list._head = _prev
                return

            ptr_pre = self._list._head
            while ptr_pre and ptr_pre._next is not self:
                ptr_pre = ptr_pre._next

            ptr_pre._next = _prev

        def __str__(self):
            return str(self._data)


    def __init__(self):
        self._head = None
        self._tail = None

    def perge(self):
        self._head = None
        self._tail = None

    @property
    def head(self):
        return self._head

    @property
    def tail(self):
        return self._tail

    @property
    def first(self):
        return self._head.data if self._head else None

    @property
    def last(self):
        return self._tail.data if self._tail else None

    @property
    def isempty(self):
        return False if self._head else True

    def prepend(self, item):
        elem = self.Element(self, item, self._head)

        if not self._tail:
            assert self._head is None, "head should be null"
            self._tail = elem

        self._head = elem

    def append(self, item):

        elem = self.Element(self, item, None)

        if not self._tail:
            assert self._head is None, "head should be null"
            self._head = elem

        else:
            self._tail._next = elem

        self._tail = elem

    def __copy__(self):

        _l = LinkedList()

        ptr = self._head

        while ptr:
            _l.append(_l.Element(_l, ptr.data, None))
            ptr = ptr.next

        return _l

    def extract(self, item):

        ptr = self._head
        pre_ptr = None

        while ptr and item is not ptr and item is not ptr.data:
            pre_ptr = ptr
            ptr = ptr._next

        if not ptr:
            raise KeyError

        if ptr is self._head:
            self._head = ptr._next
        else:
            assert pre_ptr is not None, "pre_ptr should never be null"
            pre_ptr._next = ptr._next

        if self._tail is ptr:
            self._tail = pre_ptr

    def __str__(self):

        ptr = self.head

        _str = "[ "

        if ptr:
            _str += str(ptr)

            while ptr._next:
                _str += ", " + str(ptr.next)
                ptr = ptr._next

        _str += " ]"

        return _str


def test_list():

    l = LinkedList()
    print(l)

    l.append(11)
    l.append(12)
    
    print(l)
    l.prepend(10)
    l.prepend(9)
    print(l)

    l.extract(10)
    print(l)
    print(l.first)
    print(l.last)

    _h = l.head
    _h.insert_after(30)
    print(l)
    print(l.first)
    print(l.last)

    _h.insert_before(20)
    print(l)
    print(l.first)
    print(l.last)

    _h.insert_after(8)
    print(l)
    print(l.first)
    print(l.last)

    l.extract(8)
    print(l)
    print(l.first)
    print(l.last)

    _t = l.tail
    _t.insert_after(23)
    print(l)
    print(l.first)
    print(l.last)

    _t.insert_before(22)
    print(l)
    print(l.first)
    print(l.last)

    _t.insert_after(21)
    print(l)
    print(l.first)
    print(l.last)


if __name__ == '__main__':
    test_list()

