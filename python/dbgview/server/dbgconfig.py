
import threading

class DbgConfig:
    def __init__(self, mq_gui, mq_filter, *kargs, **kwargs):

        self.mutex = threading.Lock()
        self.mq_gui = mq_gui
        self.mq_filter = mq_filter
