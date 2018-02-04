#!/usr/bin/env python3


import os
import sys
import enum
import threading
import collections


FileType = enum.Enum(
    value="FileType",
    names="file directory link"
)

CompareResult = enum.Enum(
    value="CompareResult",
    names="equal new missing diff"
)

class FileNode:

    def __init__(self, name, parent=None, filetype=FileType.file, st=CompareResult.new):
        self._parent = None
        self._child  = {}
        self._type   = filetype
        self._st     = st
        self._name   = name

#    def __len__(self):
#        return len(self._child)
    
    @property
    def len(self):
        return len(self._child)

    def __setitem__(self, key, value):

        assert self._type is FileType.directory, "current type should be directory"
        self._child[key] = value

    def __getitem__(self, key):
        assert self._type is FileType.directory, "current type should be directory"
        return self._child[key]

    def get(self, key, default=None):
        return self._child.get(key, default)

    def items(self):
        return self._child.items()

    def keys(self):
        return self._child.keys()

    def values(self):
        return self._child.values()

    def __str__(self):

        smb = {
                CompareResult.new: '\033[01;32m+',
                CompareResult.missing: '\033[00;31m-',
                CompareResult.equal: '=',
                CompareResult.diff: '\033[01;31mX'
            }

       # [\033[01;32m\] \[033[00m\]

        return "{} {}\033[00m".format(smb.get(self._st, 'X'), self._name)

    def __iter__(self):
        self._cur = iter(self._child)
        return self

    def __next__(self):
        return next(self._cur)


class DirectoryContainer:

    def __init__(self, path='.'):

        if path:
            self.load(path)

    def update(self, parent=None, path=None):

        def get_file_type(path):

            cmds = {
                FileType.file: os.path.isfile,
                FileType.directory: os.path.isdir,
                FileType.link: os.path.islink
            }

            for _ty, _cmd in cmds.items():
                if _cmd(path): return _ty

            return FileType.file


        for item in os.listdir(path):

            filetype = get_file_type("{}{}{}".format(path, os.sep, item))

            key = item

            if filetype is FileType.directory:
                key += os.sep

            parent[key] = FileNode(item, parent=parent, filetype=filetype)

        for key, node in parent.items():
            if node._type is FileType.directory:
                self.update(node, "{}{}{}".format(path, os.sep, node._name))


    def load(self, path='.'):
        if not path:
            return None

        self._root = None
        self._root = FileNode(path, 
                              filetype=FileType.directory, 
                              st=CompareResult.equal)
        self.update(self._root, path)

    @staticmethod
    def add_nodes(n_all, n_root, n_from, f_st):

        for fl in n_from:

            node = n_all[fl]

            key = node._name
            if node._type is FileType.directory:
                key += os.sep

            n_root[key] = FileNode(node._name, 
                                   parent = n_root,
                                   filetype=node._type, 
                                   st=f_st)

    @staticmethod
    def diff_dir(n1, n2, parent=None):

        assert n1 or n2, "should never both be null"

        a = set(n1.keys())
        b = set(n2.keys())

        a_b = a | b

        _same = a & b
        _miss = a_b - b
        _new  = a_b - a

        _nall = collections.ChainMap(n1, n2)

        if n1 and n2:
            _st = CompareResult.equal
        elif n1:
            _st = CompareResult.missing
        else:
            _st = CompareResult.new

        _root = FileNode(n2._name if n2 else n1._name, 
                         parent=parent,
                         filetype=FileType.directory,
                         st=_st)

        if parent:
            parent[_root._name + os.pathsep] = _root

        DirectoryContainer.add_nodes(_nall, _root, _same, CompareResult.equal)
        DirectoryContainer.add_nodes(_nall, _root, _miss, CompareResult.missing)
        DirectoryContainer.add_nodes(_nall, _root, _new,  CompareResult.new)

        for item in a_b:
            node = _nall[item]

            if node._type is FileType.directory:
                DirectoryContainer.diff_dir(n1.get(item, {}), 
                                            n2.get(item, {}),
                                            _root) 

        return _root
        

    def __sub__(self, other):

        _new = DirectoryContainer()
        _new._root = self.diff_dir(self._root, other._root)
        return _new

    def diff(self, other):
        return self.__sub__(other)

    def _dir_node_str(self, node, deepth=1, sep='\t'):

        if not node.len:
            return ""

        _str = os.linesep + (deepth-1)*sep + '|--> [' + os.linesep

        for _k, _n in sorted(node.items(), key=(lambda a: a[1]._type.value)):

            _str += deepth*sep + "|--> "
            _str += str(_n)

            if _n._type is FileType.directory:
                _str += os.sep
                _str += "{}".format(self._dir_node_str(_n, deepth + 1))

            _str += os.linesep

        _str += (deepth-1) * sep + '|--> ]'

        return _str

    def __str__(self):
        return self._dir_node_str(self._root)


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
        thread = threading.Thread(target=_loading, args=(dirc, item))
        thread.start()

        dirs[thread] = (dirc, item)

    for t in dirs:
        t.join()

#    for items in dirs.values():
#        print(items[0])

    roots = [ item[0] for item in dirs.values() ]

    #print("comparing result: ")
    print(roots[0] - roots[1])
