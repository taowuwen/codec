
import time

class DbgMessageInvalidType(Exception): pass

class DbgMessage:

    def __init__(self, ID, client, server, msg):
        self.id     = ID
        self.ctx    = msg
        self.client = client
        self.server = server
        self.tm     = time.time()
        self.prefix = ""
        '''
        time.ctime(time.time())
        '''

    def __str__(self):
        if self.prefix:
            return self.prefix + " "*30 + self.ctx

        return self.ctx

    def __repr__(self):
        return self.__str__()

    def lower(self):
        return self.ctx.lower()

    def __eq__(self, obj):

        if isinstance(obj, str):
            return self.ctx == msg

        elif isinstance(obj, DbgMessage):
            return self.ctx == obj.ctx

        raise DbgMessageInvalidType(f'{type(obj)}, {obj}, not support current type of class')


    def __contains__(self, obj):

        if isinstance(obj, str):
            return obj in self.ctx

        raise DbgMessageInvalidType(f'{type(obj)}, {obj}, not support current type of class')


    def __len__(self):
        return len(self.ctx)
