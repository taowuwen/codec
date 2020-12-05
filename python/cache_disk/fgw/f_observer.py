
class FileObserveObject:

    def __init__(self):
        self._observers = []

    def notify(self, *args, **kwargs):
        for obs in self._observers:
            try:
                obs.update(*args, **kwargs)
            except Exception as e:
                logger.error(f'Exception error: {e} while handle {obs}.update, {args} {kwargs}')
                continue

    def register(self, obs):
        if obs not in self._observers:
            self._observers.append(obs)

    def unregister(self, obs):
        try:
            self._observers.remove(obs)
        except ValueError:
            pass

class FileObserver:
    
    def update(self, *args, **kwargs):
        logger.error(f'{self.__class__.__name__} update {args}, {kwargs}, do not show this line')

    def subscribe(self, obj):
        obj.register(self)

    def unsubscribe(self, obj):
        obj.unregister(self)

class FileObject(FileObserver, FileObserveObject):
    pass

