
import threading
import sysv_ipc
from f_msg import CommandMsg
from f_event import FGWEvent, FGWEventFactory

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
            self._mq = sysv_ipc.MessageQueue(0x1000, flags=sysv_ipc.IPC_CREX, max_message_size=8192)
        except sysv_ipc.ExistentialError as e:
            self._mq = sysv_ipc.MessageQueue(0x1000)

    def run(self):

        while True:
            _msg, _type = self._mq.receive()

            _msg = _msg.decode().split()
            print(f'Req: Type: {_type}, msg:{_msg}')

            if _msg:
                evt = FGWEvent(_msg[0], CommandMsg(self.unkown_cmd, _msg))
            else:
                evt = FGWEvent('CmdUnkown', CommandMsg(self.unkown_cmd, _msg))

            self.mq_fgw.put_level1(evt)

    def unkown_cmd(self, msg):
        self._mq.send(f'Error: Unkown Command: {msg}')

    def cmd_rsp(self, msg):
        '''
         use msg result
        '''
        self._mq.send(f'{msg.result}')
