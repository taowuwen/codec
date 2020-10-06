
import enum
from stat import S_IFDIR, S_IFLNK, S_IFREG
import os

FileType = enum.Enum(
    value = 'FileType',
    names = 'DIR FILE LINK'
)

class FileNode:
    _type = FileType.FILE

    def get_file_mode(self, mode):
        _m_table = {
            FileType.DIR: S_IFDIR,
            FileType.FILE: S_IFREG,
            FileType.LINK: S_IFLNK
        }
        return (mode | _m_table.get(self._type))

    def __init__(self, name, mode):
        self._name = name
        self._parent = None

        '''
            file extension infomartions:
            1. file on disk(which disk)
            2. file on memory(which memory)
            3. file on cache(which cache)
            4. fd, current open flag handle
        '''
        self._ext  = {
            hdd = [],
            ssd = []
            memory = [],
            fd = None,
        }

        self._stat = {
            st_mode = self.get_file_mode(mode)
            st_nlink = 1
            st_size = 0
            st_ctime = time(),
            st_mtime = time(),
            st_atime = time(),
            st_uid   = os.getuid(),
            st_gid   = os.getgid(),
        }

    def __str__(self):
        return f'{self._name}'

    @property
    def name(self):
        return self._name

    @property
    def stat(self):
        return self._stat

    @property
    def ext(self):
        return self._ext

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, val):
        self._parent = val

class FileNodeFile(FileNode):
    _type = FileType.FILE

class FileNodeDir(FileNode):
    _type = FileType.DIR
    def __init__(self, name):
        super().__init__(name, 0o755)
        self._st_nlink = 2
        self._files = {}
    
    def add_file(self, fl):
        assert isinstance(fl, FileNode), 'Never show up this line'
        self._files[fl.name] = fl

class FileNodeLink(FileNode):
    _type = FileType.LINK

class FileSystem:
    '''
        file system for our cache disk
    '''
    _inst = None
    def __new__(cls, *kargs, **kwargs):

        if not cls._inst:
            cls._inst = super().__new__(cls, *kargs, **kwargs)

        return cls._inst

    def __init__(self):
        self._uid = os.getuid()
        self._gid = os.getgid()
        self._root = FileNodeDir('/', 0o755)
        self._root.parent = self._root

    def find_file(self, path):
        pass

    def create(self, path, mode):
        pass

    def mkdir(self, path, mode):
        pass

class FileActiveTable(dict):
    def __init__(self, *args, **kwargs):
        super().__init__(self, *args, **kwargs)

file_system = FileSystem()
file_active_table = FileActiveTable()
