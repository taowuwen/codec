

class BuildFailed(Exception): pass

class Factory:

    def __init__(self, name):
        self._name = name
        self.products = {}


    def register(self, key, val):
        self.products[key] = val

    def unregister(self, key):
        if key in self.products:
            self.products.pop(key)

    def get(self, key):
        return self.products.get(key, None)


class BuildFactory(Factory):

    def create(self, key, *args, **kwargs):
        try:
            return self.get(key)(*args, **kwargs)
        except Exception as e:
            print(f"Error, Unkown product {key}")
            raise BuildFailed(f"unkown product {key}")


class BuildFactoryAutoRegister(BuildFactory):
    def __init__(self, name, klasses):
        super().__init__(name)
        self.do_register(klasses)

    def do_register(self, klasses):

        for klass in klasses:
            for _cls in klass.__subclasses__():
                self.register(_cls.__name__, _cls)
