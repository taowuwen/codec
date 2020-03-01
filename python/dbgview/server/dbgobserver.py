
class Observable:

    def __init__(self, *args, **kwargs):
        self.observers = set()

    def subscribe(self, obj):
        if obj not in self.observers:
            self.observers.add(obj)

    def unsubscribe(self, obj):

        if obj in self.observers:
            self.observers.pop(obj)


    def notify(self, *args, **kwargs):

        for ob in self.observers:
            try:
                ob.update(*args, **kwargs)
            except Exception as e:
                print("ERROR --->",  e)
                return False
        return True


class Observer:

    def __init__(self, *args, **kwargs):
        pass

    def update(self, *args, **kwargs):
        pass

    def observe(self, obs):
        obs.subscribe(self)

    def no_observe(self, obs):
        obs.unsubscribe(self)

