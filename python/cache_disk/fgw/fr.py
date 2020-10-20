from f_event import fgwevent_factory, FGWEvent
from f_observer import FileObserver

class FileRouter(FileObserver):
    '''
    1. active table
    2. default routes
    3. constant hash
    '''

    def __init__(self):
        '''
        register event handler for all disk handle
        '''

    def update(self, *args, **kwargs):
        print(f'FR recved update {args}, {kwargs}')
        

    def reg_events(self, events = (), method = None):
        for evt in events:
            fgwevent_factory.register(evt, method)
         

    def reg_fuse_evt(self):
        evts = (
            'chmod', 'chown', 'create', 'mkdir', 'open',
            'read', 'rename', 'rmdir', 'symlink', 'readlink',
            'truncate', 'unlink', 'utimens', 'write'
        )
        self.reg_events(evts, self.handle_fuse_evt)

    def handle_fuse_evt(self, msg):
        print(f'handle fuse msg: {msg}')
        msg.release()

    def reg_disk_evt(self):
        pass

    def handle_disk_evt(self, msg):
        pass

    def reg_memory_evt(self):
        pass

    def handle_memory_evt(self, msg):
        pass
