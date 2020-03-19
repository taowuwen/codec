
import threading
from dbgmsg import DbgMessage
from dbgactiondef import dbg_print


class DbgData:
    def __init__(self, mq_gui, *kargs, **kwargs):
        self.mutex = threading.Lock()
        self.mq_gui = mq_gui
        self.msgs = {}

    def set(self, msg):
        '''
            thread is filter: WRITE ONLY + notify
        '''

        if not isinstance(msg, DbgMessage):
            self.mq_gui.put(msg)

        with self.mutex:
            self.msgs[msg.id] = msg

        self.mq_gui.put(msg.id)

    def get(self, msg_id):
        msg = None
        '''
            thread is gui: READ only
        '''
        with self.mutex:
            msg = self.msgs.get(msg_id, None)

        return msg

    def clear(self):
        with self.mutex:
            self.msgs.clear()

    def show(self):
        dbg_print(f'Data cache size {len(self.msgs)}')

    def delete(self, k):
        with self.mutex:
            try:
                self.msgs.pop(k)
            except Exception as e:
                dbg_print(f"invalid key {k}, not exist")
