
import queue
import threading
from dbgactiondef import ActionType, ActionTarget, ActionFilterType, action_filter_type, action_target_type, dbg_print

class DbgFilterThread(threading.Thread):

    def __init__(self, *kargs, **kwargs):

        self.dbg_data, self.mq_filter, self.actionctl, *_ = kargs
        self._run = False
        self._total_accepted = 0
        self._total_dropped  = 0
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
                    self._total_accepted += 1
                else:
                    del msg
                    self._total_dropped += 1
            except queue.Empty:
                pass
            except KeyboardInterrupt:
                break

        self._run = False
        print("filter exited")

    def stop(self):
        print("stop filter")
        self._run = False

    def dbg_clear(self):
        self._total_accepted = 0
        self._total_dropped  = 0

    def dbg_show(self):
        dbg_print(f'Filter Accepted: {self._total_accepted}, Dropped: {self._total_dropped}')
