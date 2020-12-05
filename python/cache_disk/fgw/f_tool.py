
import threading
import sysv_ipc
from f_msg import CommandMsg
from f_event import FGWEvent, FGWEventFactory
from cache_disk import fgwtool_in, fgwtool_out, logger

class FileTool(threading.Thread):

    def __init__(self, queue, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.mq_fgw = queue
        self.build_mq()

        FGWEventFactory().register('CmdUnkown', self.unkown_cmd)
        FGWEventFactory().register('CmdRsp',    self.cmd_rsp)

    def build_mq(self):
        try:
            self._mq_rx = sysv_ipc.MessageQueue(fgwtool_in, flags=sysv_ipc.IPC_CREX, max_message_size=8192)
        except sysv_ipc.ExistentialError as e:
            self._mq_rx = sysv_ipc.MessageQueue(fgwtool_in)

        try:
            self._mq_tx = sysv_ipc.MessageQueue(fgwtool_out, flags=sysv_ipc.IPC_CREX, max_message_size=8192)
        except sysv_ipc.ExistentialError as e:
            self._mq_tx = sysv_ipc.MessageQueue(fgwtool_out)

    def run(self):

        while True:
            _msg, _type = self._mq_rx.receive()

            _msg = _msg.decode().split()
            logger.debug(f'Req: Type: {_type}, msg:{_msg}')

            msg = CommandMsg(*_msg)
            msg.proc = self.unkown_cmd

            evt = _msg[0] if _msg else 'CmdUnkown'

            self.mq_fgw.put_level1(FGWEvent(evt, msg))

    def unkown_cmd(self, msg):
        self._mq_tx.send(f'Error: Unkown Command: {msg}')

    def cmd_rsp(self, msg):
        '''
         use msg result
        '''
        self._mq_tx.send(f'{msg.result}')
