
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
            "gui_font": DbgDict({ fg="black", bg="white", font="" })

            "enable_color": True,
            "enable_filter": True,

            'common': {
                "show_line_number": False,
                "show_timestamp":   False,
                "show_client":      False,
                "show_server":      False,
                "show_content_length": False,
            },

            'postconfig': {
                "set_listbox", True,
                "enable_log",  False,
            }


            "color_rule": [
                {"enable":True, "name": "rule_name1", fg='#0xaabbcc', bg='white', rule="rule priciple", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True},
                {"enable":True, "name": "rule_name2", fg='#0xaabbcc', bg='white', rule="rule priciple", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True},
            ],

            "filter_default_target": "accept",

            "filter_rule": [
                ["enable":True, "name": "rule_name1", fg='#0xaabbcc', bg='white', rule="rule priciple", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True, target="drop"],
                ["enable":True, "name": "rule_name2", fg='#0xaabbcc', bg='white', rule="rule priciple", match_condition="equal/not_euqal/contain/not_contain" , ignorecase=True, target="accept"],
                ["enable":True, "name": "rule_name1", fg='#0xaabbcc', bg='white', rule="rule priciple", match_condition="contain" , ignorecase=True, target="accept"],
            ]
        }
        '''

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
        self.kwargs['filter'] = [
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

    pprint(config.common.show_line_number)

    config.common.show_line_number = True
    pprint(config.common.show_line_number)

    config.common.tmpkey = "tmpval"
    pprint(config.common.tmpkey)

    key = 'tmpkey'

    print(config.common.get(key))

