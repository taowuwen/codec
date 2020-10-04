
import threading
import sysv_ipc

class FileTool(threading.Thread):

    def __init__(self, queue, *args, **kwargs):
        self._queue = queue

        self._msg_queue = sysv_ipc.MessageQueue(0x1000, flags=sysv_ipc.IPC_CREX, max_message_size=8192)
        super().__init__(*args, **kwargs)


    def run(self):

        while True:
            msg = self._msg_queue.receive()
            print(msg)

        
