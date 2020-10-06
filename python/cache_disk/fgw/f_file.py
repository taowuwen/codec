
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

    def __init__(self, path, mode):
        self._path = path
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
        return f'{self._path}'

    @property
    def stat(self):
        return self._stat

class FileNodeFile(FileNode):
    _type = FileType.FILE

class FileNodeDir(FileNode):
    _type = FileType.DIR
    def __init__(self, path, mode):
        super().__init__(path, mode)
        self._st_nlink = 2
        self._files = {}

class FileNodeLink(FileNode):
    _type = FileType.LINK

def file_mkdir(path):
    pass

def file_create(path):
    pass


class FileManagement:
    '''
        file management for our cache disk
    '''
    _inst = None
    def __new__(cls, *kargs, **kwargs):

        if not cls._inst:
            cls._inst = super().__new__(cls, *kargs, **kwargs)

        return cls._inst

    def __init__(self):
        self._uid = os.getuid()
        self._gid = os.getgid()
        self._root = FileNodeDir('/')

