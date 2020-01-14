
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
        '''
        {
            "gui" : DbgDict(
                "font": DbgDict({
                    fg="black",
                    bg="white",
                    font=""
                }),
            )
            'preconfig': {
            },

            'common': {
                "show_line_number": False,
                "show_timestamp":   False,
                "show_client":      False,
                "show_server":      False,
                "show_content_length": False,
            },

            "postconfig": {
                "set_listbox", True,
                "enable_log",  False,
            },

            "color": {
                "rule_name1": { fg='#0xaabbcc', bg='white', rule="rule priciple", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True, binded_msgs=[]},
                "rule_name2": { fg='#0xaabbcc', bg='white', rule="rule priciple", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True, binded_msgs=[]},
                ...
            }

            "filter": {
                "filter1": { rule="rule priciple", target="drop/continue/accept/return", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True},
                "filter2": { rule="rule priciple", target="drop/continue/accept/return", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True},
                ...
            }
        }
        '''

        self.kwargs['preconfig'] = DbgDict()
        self.kwargs['common'] = DbgDict({
            "show_line_number":    True,
            "show_timestamp":      True,
            "show_client":         False,
            "show_server":         False,
            "show_content_length": False,
        })

        self.kwargs['postconfig'] = DbgDict({
            "set_listbox": True,
            "enable_log":  False,
        })

        self.kwargs['color'] = [ 
            DbgDict({
                "name": "default",
                "rule":  '',
                "fg":   'red',
                "match_condition": "contain",
                "ignorecase":      False
            }),
        ]
        self.kwargs['filter'] = [
            DbgDict({
                "name": "default",
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

    pprint(config.common.show_line_number)

    config.common.show_line_number = True
    pprint(config.common.show_line_number)

    config.common.tmpkey = "tmpval"
    pprint(config.common.tmpkey)

    key = 'tmpkey'

    print(config.common.get(key))
