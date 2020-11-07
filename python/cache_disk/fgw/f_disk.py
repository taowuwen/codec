
import threading
import enum
from f_queue import FilePriorityQueue
from f_observer import FileObserveObject
import os
from f_event import FGWEventFactory, FGWEvent
from f_msg import *

class DiskExist(Exception): pass
class DiskNotExist(Exception): pass
class DiskNotSupportForNow(Exception): pass
class DiskInvalidArgument(Exception): pass
class DiskAlreadyStarted(Exception): pass

DiskType = enum.Enum(
    value = 'DiskType',
    names = 'HDD SSD MEMORY'
)

DiskStatus = enum.Enum(
    value = 'DiskStatus',
    names = 'INIT SCANNING RUNING STOPPED ERROR'
)

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
                evt.proc()
            except Exception as e:
                print(f'Exception on handle {evt}, {e}')
                evt.msg.result = (-1, f'{e}')
                self.disk.queue.put_msg(FGWEvent('DiskRsp', evt.msg))


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

    def __str__(self):
        return f'({self._type}:>{self._root} {self._size})'

    def create_msg(self, *args):
        return UnkownMsg(*args)

    def phy2fuse(self, path):
        assert len(path) > len(self._root), 'never show up this line'
        return path[len(self._root):]

    def fuse2phy(self, path):
        return self._root + path

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
        return MMDMsg(*aargs)


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
        dev = self.find_disk(root_dir)
        if dev:
            raise DiskExist(f'disk: {dev} existed')

        dev = HDDDisk(self.mq_fgw, root_dir, *args, **kwargs)
        self.hdd.append(dev)
        self.hdd.append(dev)

        self.notify('disk_add', 'hdd', dev)
        dev.do_start()

    def create_ssd_disk(self, root_dir, *args, **kwargs):
        raise DiskNotSupportForNow(f'ssd not support for currently')

    def create_memory_disk(self, root_dir, *args, **kwargs):
        dev = self.find_disk(root_dir)
        if dev:
            raise DiskExist(f'Memory disk: {dev} existed')

        mem = MemoryDisk(*args, **kwargs)
        self.memory.append(mem)
        self.notify('disk_add', 'mem', mem)

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
