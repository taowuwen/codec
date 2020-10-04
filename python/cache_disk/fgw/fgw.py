

class FGW:

    def __init__(self, queue):
        self._queue = queue
        self._run   = 0

    def run(self):

        self._run = 1

        while self._run:
            msg = self._queue.get_msg()
            try:
                msg.proc()
            except Exception as e:
                print(f'Exception on handle {msg}, {e}')
