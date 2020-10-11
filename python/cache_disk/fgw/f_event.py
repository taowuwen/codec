
import enum

class EventInvalidArgument(Exception): pass

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
        return self._evt_cb.get(key)

fgwevent_factory = FGWEventFactory()

class FGWEvent:
    def __init__(self, evt, msg):
        self._proc = FGWEventFactory().get_proc(evt)
        if not self._proc:
            self._proc = msg.proc
        self._evt = evt
        self._msg  = msg

    def register(self, key, val):
        self._evt.update({key: val})

    def unregister(self, key):
        self._evt.pop(key)

    def proc(self):
        self._msg.event = self._evt
        self._proc(self._msg)

    def __str__(self):
        return f'{self._proc} {self._msg}'

    def __repr__(self):
        return str(self)

