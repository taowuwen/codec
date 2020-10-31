
from f_event import FGWEventFactory, FGWEvent
from f_msg import *
from f_fuse import f_fuse_start, f_fuse_stop

class InvalidArgument(Exception):
    pass

class FGWCommand:
    _inst = None
    _mq_fgw = None

    def __new__(cls, *kargs, **kwargs):
        if not cls._inst:
            cls._inst = super().__new__(cls)

        return cls._inst

    def __init__(self, fgw = None, tool = None, *kargs, **kwargs):
        self._fgw = fgw
        self._tool = tool

    @property
    def mq_fgw(self):
        return self._mq_fgw

    @mq_fgw.setter
    def mq_fgw(self, mq):
        self._mq_fgw = mq

    def disk_handler(self, msg):
        print(f'handle disk request, {msg}')
        msg.result = (0, 'OK')
        self._mq_fgw.put_msg(FGWEvent('CmdRsp', msg))

    def fuse_handler(self, msg):
        print(f'handle fuse request, {msg}')
        try:
            if msg.msg[1] == 'start':
                f_fuse_start(msg.msg[2])
            elif msg.msg[1] == 'stop':
                f_fuse_stop()
            else:
                raise InvalidArgument('fuse invalid argument')

        except Exception as e:
            msg.result = (-1, 'Failed, {}'.format(e))
            self._mq_fgw.put_msg(FGWEvent('CmdRsp', msg))
        else:
            msg.result = (0, 'OK')
            self._mq_fgw.put_msg(FGWEvent('CmdRsp', msg))

    def memory_handler(self, msg):
        print(f'handle memory request, {msg}')
        msg.result = (-1, 'Failed')
        self._mq_fgw.put_msg(FGWEvent('CmdRsp', msg))

    def ssd_handler(self, msg):
        print(f'handle ssd request, {msg}')
        msg.result = (-1, 'failed')
        self._mq_fgw.put_msg(FGWEvent('CmdRsp', msg))

    def register_all(self):
        evt_handler = {
            'ssd':  self.ssd_handler,
            'disk': self.disk_handler,
            'fuse': self.fuse_handler,
            'mem':  self.memory_handler,
        }

        for key, val in evt_handler.items():
            FGWEventFactory().register(key, val)

fgwcommand = FGWCommand()
