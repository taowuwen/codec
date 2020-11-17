
import enum
from f_exception import *

class FGWEventFactory:
    _inst = None
    _evt_cb = {}

    def __new__(cls, *kargs, **kwargs):

        if not cls._inst:
            cls._inst = super().__new__(cls, *kargs, **kwargs)

        return cls._inst

    def register(self, key, val):
        if not hasattr(val, '__call__'):
            raise EventInvalidArgument(f'{key}:{val}')

        self._evt_cb.update({key: val})

    def unregister(self, key):
        self._evt_cb.pop(key)

    def get_proc(self, key):
        return self._evt_cb.get(key, self.unknown_evt)

    def unknown_evt(self, msg, *args, **kwargs):
        print(f'Error, did not register handler for event: {msg.event}, msg: {msg} {args} ,{kwargs}')
        raise EventUnregisted(f'event {msg.event} unregisted yet')

    def show_all(self):
        for evt,cb in self._evt_cb.items():
            print(f'{evt:>20} = {cb}')

fgwevent_factory = FGWEventFactory()

class FGWEvent:
    def __init__(self, evt, msg):
        self._proc = FGWEventFactory().get_proc(evt)
        if not self._proc:
            self._proc = msg.proc
        self._evt = evt
        self._msg  = msg
    
    @property
    def event(self):
        return self._evt

    @property
    def msg(self):
        return self._msg

    def proc(self, *args, **kwargs):
        self._msg.event = self._evt
        self._proc(self._msg, *args, **kwargs)

    def __str__(self):
        return f'<{self._evt}: {self._proc}>'

    def __repr__(self):
        return str(self)

    def __lt__(self, other):
        return False

