
import threading
from pprint import pprint

class DbgDict(dict):

    def __getattr__(self, key):
        return self.__getitem__(key)

    def __setattr__(self, key, val):
        return self.__setitem__(key, val)


class DbgConfig:

    _inst = None

    def __new__(cls, *args, **kwargs):

        if not cls._inst:
            cls._inst = super().__new__(cls, *args, **kwargs)

        return cls._inst


    def __init__(self, *kargs, **kwargs):

        self.mutex = threading.Lock()
        self.kwargs = DbgDict()
        self.set_default_config()

        if kwargs:
            self.kwargs.update(kwargs)

    def update(self, **kwargs):
        self.kwargs.update(kwargs)

    def __getattr__(self, key):

        try:
            return super().__getattribute__(key)
        except AttributeError:
            return self.__getitem__(key)

    def __setattr__(self, key, val):
        try:
            object.__setattr__(self, key, val)
        except AttributeError:
            self.__setitem(key, val)

    def __getitem__(self, item):
        res = None

        with self.mutex:
            res = self.kwargs.get(item, None)

        return res

    def __setitem__(self, key, val):

        if key and val:
            with self.mutex:
                self.kwargs[key] = val
        
    def set_default_config(self):
        self.kwargs['gui'] = {
                },
        self.kwargs['gui_font'] = DbgDict({
            "fg":               "black",
            "bg":               "white",
            "font":             "systemSystemFont",
            "height":           10,
            "width":            10,
            "selectforeground": "white",
            "selectbackground": "#33B397"
        })

        self.kwargs['common'] = DbgDict({
            "ShowLineNumber":    True,
            "ShowTimeStamp":      True,
            "ShowClient":         False,
            "ShowServer":         False,
            "ShowLength": False,
        })

        self.kwargs['postconfig'] = DbgDict({
            "EnableListbox": True,
            "EnableLog":  False,
        })

        self.kwargs['Color'] = [ 
            DbgDict({
                "enable":          True,
                "name":            "red",
                "rule":            'red',
                "fg":              'red',
                "match_condition": "contain",
                "ignorecase":      True
            }),
            DbgDict({
                "enable":          True,
                "name":            "blue",
                "rule":            'blue',
                "fg":              'blue',
                "match_condition": "contain",
                "ignorecase":      False
            }),

            DbgDict({
                "enable":          True,
                "name":            "BigBlue",
                "rule":            'BLUE',
                "fg":              '#00aaaa',
                "match_condition": "contain",
                "ignorecase":      False
            }),

            DbgDict({
                "enable":          True,
                "name":            "yellow",
                "rule":            'yellow',
                "fg":              'yellow',
                "match_condition": "contain",
                "ignorecase":      True
            }),
        ]
        self.kwargs['Filter'] = [
            DbgDict({
                "enable":          True,
                "name":            "BLUE",
                "rule":            'BLUE',
                "target":          'drop',
                "match_condition": "contain",
                "ignorecase":      False
            }),
            DbgDict({
                "enable":          True,
                "name":            "filter",
                "rule":            '',
                "target":          'accept',
                "match_condition": "contain",
                "ignorecase":      False
            }),
        ]

        pprint(self.kwargs)

config = DbgConfig()

if __name__ == '__main__':
    pprint(config.preconfig)
    pprint(config.common)
    pprint(config.postconfig)
    pprint(config.filter)
    pprint(config.color)
    pprint(config.gui_font)

    pprint(config.common.ShowLineNumber)

    config.common.ShowLineNumber = True
    pprint(config.common.ShowLineNumber)

    config.common.tmpkey = "tmpval"
    pprint(config.common.tmpkey)

    key = 'tmpkey'

    print(config.common.get(key))

