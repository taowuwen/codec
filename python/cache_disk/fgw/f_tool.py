
import threading
import sysv_ipc
from f_msg import CommandMsg
from f_event import FGWEvent, FGWEventFactory
from cache_disk import fgwctl_key

class FileTool(threading.Thread):

    def __init__(self, queue, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.mq_fgw = queue
        self.build_mq()

        FGWEventFactory().register('CmdUnkown', self.unkown_cmd)
        FGWEventFactory().register('CmdRsp',    self.cmd_rsp)

        self._reqs = {}

    def build_mq(self):
        try:
            self._mq = sysv_ipc.MessageQueue(fgwctl_key, flags=sysv_ipc.IPC_CREX, max_message_size=8192)
        except sysv_ipc.ExistentialError as e:
            self._mq = sysv_ipc.MessageQueue(fgwctl_key)

    def run(self):

        while True:
            _msg, _type = self._mq.receive()

            _msg = _msg.decode().split()
            print(f'Req: Type: {_type}, msg:{_msg}')

            msg = CommandMsg(_msg)
            msg.proc = self.unkown_cmd

            evt = _msg[0] if _msg else 'CmdUnkown'

            self.mq_fgw.put_level1(FGWEvent(evt, msg))

    def unkown_cmd(self, msg):
        self._mq.send(f'Error: Unkown Command: {msg}')

    def cmd_rsp(self, msg):
        '''
         use msg result
        '''
        self._mq.send(f'{msg.result}')