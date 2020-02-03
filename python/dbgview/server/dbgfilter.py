
import queue
import threading
from dbgactiondef import ActionType, ActionTarget, ActionFilterType, action_filter_type, action_target_type

class DbgFilterThread(threading.Thread):

    def __init__(self, *kargs, host='localhost', port=23232, **kwargs):

        self.addr = (host, port)
        self.dbg_data, self.mq_filter, self.actionctl, *_ = kargs
        self._run = False
        super().__init__()

    def run(self):

        print("filter started")
        self._run = True

        while self._run:
            try:
                msg = self.mq_filter.get(timeout=1)

                tgt = self.actionctl.filter_action(msg)
                if tgt is ActionTarget.ACCEPT:
                    self.dbg_data.set(msg)
                else:
                    print("filter msg: ", msg)
                    del msg
            except queue.Empty:
                pass
            except KeyboardInterrupt:
                break

        self._run = False
        print("filter exited")

    def stop(self):
        print("stop filter")
        self._run = False
