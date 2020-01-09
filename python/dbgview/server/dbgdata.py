
import threading
from dbgmsg import DbgMessage

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
            print("msg is not DbgMessage")
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
