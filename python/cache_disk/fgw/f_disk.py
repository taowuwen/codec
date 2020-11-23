
import threading
import enum
from f_queue import FilePriorityQueue
from f_observer import FileObserveObject
import os
from f_event import FGWEventFactory, FGWEvent
from f_msg import *
from f_exception import *
from cache_disk import *
from f_disk_oper import disk_mkdir_p

class DiskThread(threading.Thread):

    def __init__(self, disk):
        super().__init__()
        self.disk = disk
        self.running = False

    def run(self):

        self.running = True
        while self.running:
            evt = self.disk.get_evt()
            try:
                evt.proc(self.disk)
            except Exception as e:
                print(f'DISK: Exception({self.disk}) handle evt: {evt} --> Exceptin is {e}')
                evt.msg.result = (-1, e)
                self.disk.send_rsp_msg(msg)

class Disk:
    def __init__(self, mq_fgw, root_dir=None, size=0, *kargs, **kwargs):
        self._info = {}
        self._info.update(kwargs)
        self._mq_fgw = mq_fgw
        self._mq = FilePriorityQueue(f"mq_{root_dir}")
        self._root = root_dir
        self._size = int(size)
        self._usedsize = 0

        if not self._root or self._size <= 0:
            raise DiskInvalidArgument(f'invalid argument, root: {self._root} invalid or size: {self._size} invalid')
        self._thread_main = None
        self._thread_pool = None
        self._status = None
        print(f'{self._type} create with root: {self._root}, size: {self._size}')

    def get_evt(self):
        return self._mq.get_msg()

    @property
    def dev(self):
        return self._root

    @property
    def root(self):
        return self._root

    @property
    def queue(self):
        return self._mq_fgw

    @property
    def msg_queue(self):
        return self._mq

    @property
    def totalsize(self):
        return self._size

    @property
    def usedsize(self):
        return self._usedsize

    def do_start(self):

        if self._thread_main:
            raise DiskAlreadyStarted('disk already started')

        # send self a msg with high priority
        self._mq.put_level1(FGWEvent('disk_scan', HDDMsg(self)))
        self._thread_main = DiskThread(self)
        self._thread_main.start()

    def do_stop(self):
        if self._thread_main:
            self._thread_main.running = False
            self._thread_main.join()
            self._thread_main = None

    def do_update(self, **kwargs):
        print(f'do update {kwargs}')

    @property
    def disk_type(self):
        return self._type

    @property
    def type_name(self):
        return self.disk_type.name.lower()

    def __str__(self):
        return f'({self._type}:>{self._root} {self._size})'

    def __repr__(self):
        return self.__str__()

    def create_msg(self, *args):
        return UnkownMsg(*args)

    def phy2fuse(self, path):
        assert len(path) > len(self._root), 'never show up this line'
        return path[len(self._root):]

    def fuse2phy(self, path):
        return self._root + path

    def send_rsp_msg(self, msg):
        print(f'{self}, build response msg for event: {msg.event} on {msg}, {msg.result}')
        self.queue.put_msg(FGWEvent(f'rsp_{msg.event}', msg))

    def mkdir(self, msg, fl, mode):
        os.mkdir(fl, mode)

    def create_or_open(self, msg, fl, flags, mode=511):
        fn = msg.msg[0]

        try:
            fd = os.open(fl, flags, mode)
        except FileNotFoundError:
            print(f'file not found, do mkdir for {fl}')
            disk_mkdir_p(self, os.path.dirname(fl))
            fd = os.open(fl, flags, mode)

        _info = fn.info[self.type_name]

        _info['fd'] = fd
        _info['status'] = FileStatus.opened
        _info['disk'] = self
        _info['sync'] = 0


    def open(self, msg, fl, flags):
        return self.create_or_open(msg, fl, flags)

    def create(self, msg, fl, mode):
        return self.create_or_open(msg, fl, os.O_RDWR|os.O_CREAT , mode)

    def refresh_fd(self, msg):
        fn = msg.msg[0]
        _info = fn.info[self.type_name]
        return _info['fd']

    def truncate(self, msg, fl, length, fh):
        return os.truncate(fl, length)

    def write(self, msg, fl, data, offset, fh):
        fh = self.refresh_fd(msg)
        os.lseek(fh, offset, os.SEEK_SET)
        return os.write(fh, data)

    def read(self, msg, fl, size, offset, fh):
        fh = self.refresh_fd(msg)
        os.lseek(fh, offset, os.SEEK_SET)
        return os.read(fh, size)

    def flush(self, msg, fl, fh):
        fh = self.refresh_fd(msg)
        return os.fsync(fh)

    def release(self, msg, fl, fh):
        fn = msg.msg[0]
        _info = fn.info[self.type_name]

        fh = _info['fd']
        os.close(fh)

        _info['fd'] = 0
        _info['status'] = FileStatus.closed
        _info['disk'] = None
        _info['sync'] = 0
        return 0

    def getxattr(self, msg, fl, name, position=0):
        return os.getxattr(fl, name)

    def listxattr(self, msg, fl):
        return os.listxattr(fl)

    def removexattr(self, msg, fl, name):
        return os.removexattr(fl, name)

    def setxattr(self, msg, fl, name, value, options, position=0):
        return os.setxattr(fl, name, value, options)

    def unlink(self, msg, fl):
        fn = msg.msg[0]

        if self in fn.ext[self.type_name]:
            fn.ext[self.type_name].remove(self)

        return os.unlink(fl)

    def chmod(self, msg, fl, mode):
        return os.chmod(fl, mode)

    def chown(self, msg, fl, uid, gid):
        return os.chown(fl, uid, gid)

    def readlink(self, msg, fl):
        return os.readlink(fl)

    def rename(self, msg, fl, tgt):
        return os.rename(fl, tgt)

class HDDDisk(Disk):
    _type = DiskType.HDD

    def create_msg(self, *args):
        return HDDMsg(*args)

class SSDDisk(Disk):
    _type = DiskType.SSD

    def create_msg(self, *args):
        return SSDMsg(*args)

class MemoryDisk(Disk):
    _type = DiskType.MEMORY

    def create_msg(self, *args):
        return MMDMsg(*args)

class DiskManager(FileObserveObject):

    def __init__(self, queue = None):
        super().__init__()

        self.hdd = []
        self.ssd = []
        self.memory = []
        self.mq_fgw = queue

    def find_disk(self, dev):
        for disk in self.hdd + self.ssd + self.memory:
            if disk.dev == dev:
                return disk

        return None

    def create_hdd_disk(self, root_dir, *args, **kwargs):
        print(f'disk create hdd disk {root_dir}, {args}, {kwargs}')
        dev = HDDDisk(self.mq_fgw, root_dir, *args, **kwargs)
        self.hdd.append(dev)

        self.notify('disk_add', dev)
        dev.do_start()

    def create_ssd_disk(self, root_dir, *args, **kwargs):
        raise DiskNotSupportForNow(f'ssd not support for currently')

    def create_memory_disk(self, root_dir, *args, **kwargs):
        mem = MemoryDisk(self.mq_fgw, root_dir, *args, **kwargs)
        self.memory.append(mem)

        self.notify('disk_add', mem)
        mem.do_start()

    def disk_create(self, disk_type, root_dir, *args, **kwargs):
        dev = self.find_disk(root_dir)
        if dev:
            raise DiskExist(f'Memory disk: {dev} existed')

        create_disk = {
            'disk': self.create_hdd_disk,
            'ssd': self.create_ssd_disk,
            'memory': self.create_memory_disk
        }.get(disk_type, None)

        if not create_disk:
            raise InvalidArgument(f'Invlaid argument for disk create {disk_type}')

        create_disk(root_dir, *args, **kwargs)

    def disk_delete(self, disk):
        dev = self.find_disk(disk)
        if dev:
            dev.do_stop()
            self.notify('disk_del', dev)

            if dev in self.hdd:
                self.hdd.remove(dev)

            elif dev in self.ssd:
                self.ssd.remove(dev)

            else:
                assert dev in self.memory, 'never show up this line'
                self.memory.remove(dev)

        return True

    def disk_update(self, disk, *args, **kwargs):
        dev = self.find_disk(disk)
        if not dev:
            raise DiskNotExist(f'disk: {disk} not exist')

        dev.do_update(args, **kwargs)
