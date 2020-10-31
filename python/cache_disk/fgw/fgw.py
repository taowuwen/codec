
from f_msg import FuseMsg, CommandMsg
from f_event import FGWEvent

class FGW:

    def __init__(self, queue):
        self._queue = queue
        self._run   = 0

    def run(self):

        self._run = 1

        while self._run:
            evt = self._queue.get_msg()
            try:
                evt.proc()
            except Exception as e:
                print(f'Exception on handle {evt}, {e}')
                if isinstance(evt.msg, FuseMsg):
                    evt.msg.release()
                elif isinstance(evt.msg, CommandMsg):
                    evt.msg.result = (-1, f'Exceptoin on handle {evt}, {e}')
                    self._queue.put_msg(FGWEvent('CmdRsp', evt.msg))


