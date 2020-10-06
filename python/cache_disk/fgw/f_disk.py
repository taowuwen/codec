
import threading
from f_queue import FilePriorityQueue


class Disk:
    def __init__(self, mq_fgw, name = 'diskname', *kargs, **kwargs):
        self._info = {}
        self._info.update(kwargs)
        self._mq_fgw = mq_fgw
        self._mq = FilePriorityQueue(f"mq_{self._info['name']}")

    @property
    def mq(self):
        return self._mq

class HDDDisk(Disk):
    pass

class SSDDisk(Disk):
    pass


class MemoryDisk(Disk):
    pass


class DiskManager:

    def __init__(self):

        self.hdd = []
        self.ssd = []
        self.memory = []
        self.mq_fgw = None


    def set_fgw_mq(self, _q):
        self.mq_fgw = _q

    def create_hdd_disk(self):
        pass

    def create_ssd_disk(self):
        pass

    def create_memory_disk(self):
        pass


