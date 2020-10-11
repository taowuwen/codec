from f_event import fgwevent_factory, FGWEvent

class FileRouter:
    '''
    1. active table
    2. default routes
    3. constant hash
    '''

    def __init__(self):
        '''
        register event handler for all disk handle
        '''

        evts = ['chmod', 'chown', 'create']

        for evt in evts:
            fgwevent_factory.register(evt, self.handle_evt)

    def handle_error(self):
        pass

    def handle_evt(self, msg):
        print('FR, handle evt: {}, msg: {}'.format(msg.evt, msg))

