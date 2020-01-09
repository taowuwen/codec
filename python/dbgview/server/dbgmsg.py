
import time

class DbgMessage:

    def __init__(self, ID, client, server, msg):
        self.id     = ID
        self.ctx    = msg
        self.client = client
        self.server = server
        self.tm     = time.time()

    def __str__(self):
        return self.ctx

    def __repr__(self):
        return self.__str__()
