
import threading
import enum
from f_queue import FilePriorityQueue
from f_observer import FileObserveObject

class DiskExist(Exception): pass
class DiskNotExist(Exception): pass
class DiskNotSupportForNow(Exception): pass

DiskType = enum.Enum(
    value = 'DiskType'
    names = 'HDD SSD MEMORY'
)

DiskStatus = enum.Enum(
    value = 'DiskStatus'
    names = 'INIT SCANNING RUNING STOPPED ERROR'
)

class DiskThread(threading.Thread):

    def __init__(self, disk):
        super().__init__()
        self.disk = disk
        self.running = 0

    def run(self):

        self.running = 1
        while self.running:
            evt = self.disk.get_evt()
            try:
                evt.proc()
            except Exception as e:
                print(f'Exception on handle {evt}, {e}')
                evt.msg.result = (-1, f'{e}')
                self.disk.queue.put_msg(FGWEvent('DiskRsp', evt.msg))


class Disk:
    def __init__(self, mq_fgw, dev = None, root_dir=None, *kargs, **kwargs):
        self._info = {}
        self._info.update(kwargs)
        self._mq_fgw = mq_fgw
        self._mq = FilePriorityQueue(f"mq_{self._info['name']}")
        self._dev = dev
        self._root = root_dir
        self._thread_main = None
        self._thread_pool = None
        self._status = None

    def get_evt(self):
        return self._mq.get_msg()

    @property
    def queue(self):
        return self._mq_fgw

    def do_start(self):
        pass

    def do_stop(self):
        pass

    def do_update(self, **kwargs):
        pass

    @property
    def disk_type(self):
        return self._type


class HDDDisk(Disk):
    self._type = DiskType.HDD
    pass

class SSDDisk(Disk):
    self._type = DiskType.SSD
    pass


class MemoryDisk(Disk):
    self._type = DiskType.MEMORY
    pass


class DiskManager(FileObserveObject):

    def __init__(self):

        self.hdd = []
        self.ssd = []
        self.memory = []
        self.mq_fgw = None

    def set_fgw_mq(self, _q):
        self.mq_fgw = _q

    def find_disk(self, dev):
        for disk in self.hdd + self.ssd + self.memory:
            if disk.dev == dev:
                return disk

        return None

    def create_hdd_disk(self, dev, *args, **kwargs):
        dev = self.find_disk(dev)
        if dev:
            raise DiskExist(f'disk: {dev} existed')

        dev = HDDDisk(dev, *args, **kwargs)
        self.hdd.append(dev)
        self.notify('disk_add', 'hdd', dev)

    def create_ssd_disk(self, dev, *args, **kwargs):
        raise DiskNotSupportForNow(f'ssd not support for currently')

    def create_memory_disk(self, dev, *args, **kwargs):

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
