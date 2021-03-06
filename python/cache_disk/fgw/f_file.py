
import enum
from stat import S_IFDIR, S_IFLNK, S_IFREG
import os
import time
from f_observer import FileObject
from f_exception import *
from cache_disk import *

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
        self._name = os.path.basename(path)
        self._path = path
        self._parent = None

        '''
            file extension infomartions:
            1. file on disk(which disk)
            2. file on memory(which memory)
            3. file on cache(which cache)
        '''
        self._ext  = dict(
            hdd = [],
            ssd = [],
            memory = [],
        )

        self._info = dict(
            hdd    = dict(status = 0, sync = 0, fd = None, disk = None),
            ssd    = dict(status = 0, sync = 0, fd = None, disk = None),
            memory = dict(status = 0, sync = 0, fd = None, disk = None)
        )

        self._stat = dict(
            st_mode = self.get_file_mode(mode),
            st_nlink = 1,
            st_size = 0,
            st_ctime = time.time(),
            st_mtime = time.time(),
            st_atime = time.time(),
            st_uid   = os.getuid(),
            st_gid   = os.getgid(),
        )

        self._xattr = {}

    @property
    def xattr(self):
        return self._xattr

    @property
    def info(self):
        return self._info

    @property
    def fd(self):
        for key in ('memory', 'ssd', 'hdd'):
            if self._info[key]['status'] is FileStatus.opened:
                assert self._info[key]['fd'] != None, 'never show up this line'
                return self._info[key]['fd']

        return -1

    def __str__(self):
        return f'{self._path}'

    def __repr__(self):
        return f'{self.__class__.__name__}({str(self)})'

    @property
    def name(self):
        return self._name

    @property
    def stat(self):
        return self._stat

    @stat.setter
    def stat(self, val):
        if isinstance(val, os.stat_result):
            self._stat['st_mode']  = val.st_mode
            self._stat['st_nlink'] = val.st_nlink
            self._stat['st_uid']   = val.st_uid
            self._stat['st_gid']   = val.st_gid
            self._stat['st_size']  = val.st_size
            self._stat['st_atime'] = val.st_atime
            self._stat['st_mtime'] = val.st_mtime
            self._stat['st_ctime'] = val.st_ctime

    @property
    def ext(self):
        return self._ext

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, val):
        self._parent = val

    def is_file(self):
        return False

    @property
    def abs_path(self):
        return self._path

    path = abs_path

    is_dir = is_file
    is_link = is_file


class FileNodeFile(FileNode):
    _type = FileType.FILE

    def is_file(self):
        return True

class FileNodeDir(FileNode):
    _type = FileType.DIR
    def __init__(self, name, mode):
        super().__init__(name, mode)
        self._st_nlink = 2
        self._files = {}
    
    def add_file(self, fl):
        assert isinstance(fl, FileNode), 'Never show up this line'
        self._files[fl.name] = fl

    def get_file(self, filename):
        return self._files[filename]

    @property
    def files(self):
        return self._files

    def is_dir(self):
        return True

    def pop(self, fl):
        return self._files.pop(fl)

class FileNodeLink(FileNode):
    _type = FileType.LINK

    def is_link(self):
        return True

class FileSystem(FileObject):
    '''
        file system for our cache disk
    '''
    _inst = None
    def __new__(cls, *kargs, **kwargs):

        if not cls._inst:
            cls._inst = super().__new__(cls, *kargs, **kwargs)

        return cls._inst

    def __init__(self):
        super().__init__()
        self._uid = os.getuid()
        self._gid = os.getgid()
        self._root = FileNodeDir('/', 0o755)
        self._root.parent = self._root
        self._cmd_table = {}
        self._disks = []

    def update_unkown_oper(self, *args, **kwargs):
        raise InvalidArgument(f'Invalid argument {args}, {kwargs}')

    def update(self, *args, **kwargs):
        logger.debug(f'file_system be updated with {args}, {kwargs}')

        cmd, *_ = args

        if not self._cmd_table:
            self._cmd_table = {
                'disk_add': self.handle_disk_add,
                'disk_del': self.handle_disk_del,
                'disk_upt': self.handle_disk_upt,
            }

        return self._cmd_table.get(cmd, self.update_unkown_oper)(*args[1:], **kwargs)

    def handle_disk_add(self, disk):
        if disk not in self._disks:
            self._disks.append(disk)

    def handle_disk_del(self, disk):
        if disk in self._disks:
            self._disks.remove(disk)

    def handle_disk_upt(self, disk):
        pass

    def find_file(self, path):

        _path = path.split(os.sep)
        while True:
            try:
                _path.remove('')
            except ValueError:
                break

        cur_node = self.root
        for fl in _path:
            try:
                cur_node = cur_node.get_file(fl)
            except KeyError:
                return None

        return cur_node

    def insert(self, fn):
        _dir = self.find_file(os.path.dirname(fn.abs_path))
        _dir.add_file(fn)
        fn.parent = _dir
        return fn

    def unlink(self, fn):
        if any(fn.ext.values()):
            return

        assert fn.parent != None, "never show up this line"

        fn.parent.pop(fn.name)
        fn.parent = None
        del fn

    def create(self, path, mode=0o755):
        return self.insert(FileNodeFile(path, mode))

    def mkdir(self, path, mode=0o755):
        return self.insert(FileNodeDir(path, mode))

    def tmpfile(self, path, mode):
        return FileNodeFile(path, mode)

    def tmpdir(self, path, mode):
        return FileNodeDir(path, mode)

    @property
    def root(self):
        return self._root

    def disk_size(self, ty):

        _total = 0
        _used  = 0

        for disk in self._disks:
            if disk.disk_type is ty:
                _total += disk.totalsize
                _used  += disk.usedsize

        return (_total, _used)

    @property
    def statfs(self):
        hdd_total, hdd_used = self.disk_size(DiskType.HDD)
        mem_total, mem_used = self.disk_size(DiskType.MEMORY)
        ssd_total, ssd_used = self.disk_size(DiskType.SSD)

        return dict(
                    f_type=0x65735546,
                    f_bsize=512,
                    f_blocks=hdd_total>>9, 
                    f_bavail=(hdd_total - hdd_used) >>9,
                    f_bfree=(hdd_total - hdd_used) >>9,
                )


file_system = FileSystem()

if __name__ == '__main__':
    import glob

    def scan_dir(path = None):
        if not path:
            return

        for fl in glob.glob(path+'/*'):
            if os.path.isdir(fl):
                file_system.mkdir(fl).stat = os.stat(fl)
                scan_dir(fl)
            else:
                file_system.create(fl).stat = os.stat(fl)

    fn_etc = file_system.mkdir('/etc')
    fn_etc.stat = os.stat('/etc')

    scan_dir('/etc')

    def _show_files(fl = file_system.root, path=None, brief=1):

        if not fl:
            return None

        if brief:
            print(path + str(fl))
        else:
            print('{:<100}:{}'.format(path + str(fl), fl.stat))

        if fl.is_dir():
            for _fl in fl.files:
                _show_files(fl.get_file(_fl), path + os.sep + _fl, brief)

    _show_files(file_system.root, '', 0)
