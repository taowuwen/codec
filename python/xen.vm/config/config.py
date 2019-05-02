#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class VMConfig:
    ext = "default"
    def __init__(self, path):
        self._path = path

    def serialize(self, cfg=None):
        pass

    def load(self):
        pass

    def read(self):

        _ctx = None

        with open(self._path, "r") as fp:
            _ctx = fp.read()

        return _ctx

    def write(self, ctx):

        with open(self._path, "w") as fp:
            fp.write(ctx)

    @property
    def path(self):
        return self._path

    @path.setter
    def path(self, val):
        self._path = val
