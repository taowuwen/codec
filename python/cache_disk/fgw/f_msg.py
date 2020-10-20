
import enum
from threading import Semaphore

FGWModule = enum.Enum(
    value = 'FGWModule',
    names = 'tool fuse hdd ssd mmd fw unkown'
)

def unkown_msg(msg):
    print(f'Error, unkown msg {msg}')

class f_msg:
    _type = FGWModule.unkown

    def __init__(self, msg = [], proc = unkown_msg, tgt = None, pos = None):
        self._pos = pos
        self._tgt = tgt
        self._result = None
        self._msg = msg
        self._proc = proc
        self._evt = None
        self._sem = Semaphore(0)

    @property
    def event(self):
        return self._evt

    @event.setter
    def event(self, evt):
        self._evt = evt

    @property
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, val):
        self._pos = pos

    @property
    def tgt(self):
        return self._tgt

    @tgt.setter
    def tgt(self, val):
        self._tgt = val

    @property
    def result(self):
        return self._result

    @result.setter
    def result(self, val):
        self._result = val

    @property
    def proc(self):
        return self._proc

    @proc.setter
    def proc(self, val):
        self._proc = val

    def __str__(self):
        return f'{self._type} -> {self._tgt}, current {self._pos}, msg: {self._msg}'

    def __repr__(self):
        return str(self)

    @property
    def type(self):
        return self._type

    @property
    def msg(self):
        return self._msg

    def wait(self, blocking=True, timeout=120):
        self._sem.acquire(blocking, timeout)

    def release(self):
        self._sem.release()

class CommandMsg(f_msg):
    _type = FGWModule.tool

class FuseMsg(f_msg):
    _type = FGWModule.fuse

class HDDMsg(f_msg):
    _type = FGWModule.hdd

class SSDMsg(f_msg):
    _type = FGWModule.ssd

class MMDMsg(f_msg):
    _type = FGWModule.mmd

class FWMsg(f_msg):
    _type = FGWModule.fw

class UnkownMsg(f_msg):
    _type = FGWModule.unkown

