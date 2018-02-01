#!/usr/bin/env python3


import os
import sys
import enum
import threading



FileType = enum.Enum(
    name="FileType",
    values="file directory link"
)


CompareResult = enum.Enum(
    names="CompareResult",
    values="equal new missing diff"
)

class FileNode:

    def __init__(self, name, parent=None, filetype=FileType.file, st=CompareResult.new):
        self._parent = None
        self._child  = {}
        self._type   = filetype
        self._st     = st
        self._name   = name


    def __setitem__(self, key, value):

        assert self._type is FileType.directory, "current type should be directory"
        self._child[key] = value

    def __getitem__(self, key):
        assert self._type is FileType.directory, "current type should be directory"
        return self._child[key]


class DirectoryContainer:

    def __init__(self, path='.'):

        if path:
            self.load(path)


    def _do_load_one(self, path=None):
        pass

    def update(self, parent=None, path=None):

        for item in os.listdir(path):

            filetype = # get file types here

            parent[item] = FileNode(item,
                                    parent=parent,
                                    filetype=filetype)


        for key, fl in parent.items():
            if fl._type is FileType.directory:
                self.update(fl, path + fl._name)

            


    def load(self, path='.'):
        if not path:
            return None

        self._root = None
        self._root = FileNode(path, filetype=FileType.directory, st=CompareResult.equal)
        self.update(self._root, path)


    def __sub__(self, other):
        pass

    def diff(self, other):
        return self.__sub__(other)


if __name__ == '__main__':

    if len(sys.argv) <= 2:
        print("usage: {} dir1 dir2".format(sys.argv[0]))
        sys.exit(1)

    if not (os.path.isdir(sys.argv[1]) and os.path.isdir(sys.argv[2])):
        print("usage: {} dir1 dir2".format(sys.argv[0]))
        sys.exit(1)

    dirs = {}

    def _loading(dirc, path):
        import time

        s = time.time()
        print('loading files in {}...'.format(path))
        dirc.load(path)
        e = time.time()
        print('loading files in {} finished, total time used: {}'.format(path, e - s))

    for item in sys.argv[1:3]:

        dirc = DirectoryContainer()
        thread = threading.Thread(target=_loading, args(dirc, item))
        thread.start()

        dirs[thread] = (dirc, item)

    for t in dirs:
        t.join()

    print("comparing result: ")
    print(dir1 - dir2)
