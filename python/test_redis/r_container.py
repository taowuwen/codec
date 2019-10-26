#!/usr/bin/env python3
# -*- coding: utf-8 -*-

class TestCaseContainer(dict):
    def register(self, key, val):
        self[key] = val

    def unregister(self, key):
        self.pop(key)

if __name__ == '__main__':
    tcc = TestCaseContainer()
    print(tcc)

    tcc.register('foo', 'bar')
    print(tcc)

    tcc.unregister('foo')
    print(tcc)
