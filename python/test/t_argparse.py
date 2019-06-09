#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import argparse
from functools import wraps


def split_function(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        print()
        print()
        print("------------------------------------------------------")
        print("-----------{:^30}-------------".format(func.__name__))
        return func(*args, **kwargs)
    return wrapper


@split_function
def _test():

    parser = argparse.ArgumentParser(description="this is my argparse test")

    parser.add_argument('-a', action='store_true', default=False)
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-c', action='store', dest='c', type=int)

    ns = parser.parse_args(['-a', '-bval', '-c', '3'])

    print(ns)
    print(ns.a)
    print(ns.b)
    print(ns.c)

@split_function
def _test_long_args():

    parser = argparse.ArgumentParser(description="my long argument test")

    parser.add_argument('--noarg', action='store_true', default=False)
    parser.add_argument('--witharg', action='store', dest='witharg')
    parser.add_argument('--withint', action='store', dest='withint', type=int)

    ns = parser.parse_args(['--noarg', '--witharg=abc', '--withint', '12'])

    if ns:
        print(ns)

@split_function
def _test_non_minus():
    parser = argparse.ArgumentParser(description="for non minus")

    parser.add_argument('count', action='store', default=3, type=int)
    parser.add_argument('uints', action='store')
    parser.add_argument('--foo', type=int)

    print(parser.parse_args(['2', 'inches', '--foo', '4']))
#    print(parser.parse_args())


@split_function
def _test_store():
    parser = argparse.ArgumentParser(description="test for store types")

    parser.add_argument('-s', 
                        action='store', 
                        dest='simple_store', 
                        help='do simple store')

    parser.add_argument('-c', 
                        action='store_const', 
                        dest='const_value', 
                        const='my const value',
                        help='do simple store')

    parser.add_argument('-t',
                        action='store_true',
                        default=False,
                        dest='arg_t',
                        help='switch to true')

    parser.add_argument('-f',
                        action='store_false',
                        default=True,
                        dest='arg_f',
                        help='switch to false')

    parser.add_argument('-a',
                        action='append',
                        default=[],
                        help='for append value')

    parser.add_argument('-A',
                        action='append_const',
                        const='value-1-to-append',
                        default=[],
                        help='for append const value 1')

    parser.add_argument('-B',
                        action='append_const',
                        const='value-2-to-append',
                        default=[],
                        help='for append const value 2')

    parser.add_argument('-v',
                        action='version',
                        version='%(prog)s 1.0')

    print(parser.parse_args())

@split_function
def _test_prefix():

    parser = argparse.ArgumentParser(description="test prefix", prefix_chars='+-/')

    parser.add_argument('-a', 
                        action='store_false', 
                        dest='a', 
                        default=None,
                        help='turn off A')

    parser.add_argument('+a',
                        action='store_true',
                        default=None,
                        dest='a',
                        help='turn on A')

    parser.add_argument('//noarg', '++noarg',
                        action='store_true',
                        help='no arg for now')

    print(parser.parse_args())



@split_function
def _test_config_parser():
    from configparser import ConfigParser
    import shlex


    parser = argparse.ArgumentParser(description='test for config parser')
    parser.add_argument('-a', action='store_true', default=False, help='turn on')
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-c', action='store', dest='c', type=int)
    parser.add_argument('-C', action='store', dest='config', default='/tmp/tmp.ini', help='config file path')

    argument = parser.parse_args()
    print(argument)

    config = ConfigParser()
    config.read(argument.config)
    config_value = config.get('cli', 'option')
    print('config_value = {}'.format(config_value))

    arg_lists = shlex.split(config_value)
    print('arg lists: {}'.format(arg_lists))

    print(parser.parse_args(arg_lists))



@split_function
def _test_config_parser_v2():
    from configparser import ConfigParser
    import shlex

    parser = argparse.ArgumentParser(description='test for config parser', fromfile_prefix_chars='@')
    parser.add_argument('-a', action='store_true', default=False, help='turn on')
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-c', action='store', dest='c', type=int)

    print(parser.parse_args(['@/tmp/config.txt']))


@split_function
def _test_helper_disabled():
    parser = argparse.ArgumentParser(description='test for config parser', add_help=False)
    parser.add_argument('-a', action='store_true', default=False, help='turn on')
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-c', action='store', dest='c', type=int)

    print(parser.parse_args())


@split_function
def _test_helper():
    parser = argparse.ArgumentParser(description='test for config parser', add_help=True)
    parser.add_argument('-a', action='store_true', default=False, help='turn on')
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-c', action='store', dest='c', type=int)

    print(parser.parse_args())

    print("********")
    parser.print_usage()

    print("=========")
    parser.print_help()


@split_function
def _test_raw_description_help_formatter():

    parser = argparse.ArgumentParser(add_help=True,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description="""
                descrption
                    not 
                        wrapped, HERE
            """,
            epilog="""
            epilog
                not
                    wrapped
            """
        )

    parser.add_argument('-a', action='store_true', default=False, help='''turn 
                on, be wrapped???
                yes
                or no
            ''')
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-c', action='store', dest='c', type=int)

    print(parser.parse_args())


@split_function
def _test_raw_text_help_formatter():

    parser = argparse.ArgumentParser(add_help=True,
            formatter_class=argparse.RawTextHelpFormatter,
            description="""
                descrption
                    not 
                        wrapped, HERE
            """,
            epilog="""
            epilog
                not
                    wrapped
            """
        )

    parser.add_argument('-a', action='store_true', default=False, help='''turn 
                on, be wrapped???
                yes
                or no
            ''')
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-c', action='store', dest='c', type=int)

    print(parser.parse_args())


@split_function
def _test_metavar_type_help_formatter():
    parser = argparse.ArgumentParser(add_help=True,
            formatter_class=argparse.MetavarTypeHelpFormatter,
            description="""
                descrption
                        wrapped, HERE
            """,
            epilog="""
            epilog
                    wrapped
            """
        )

    parser.add_argument('-a', action='store_true', default=False, help='''turn 
                on, be wrapped???
                yes
                or no
            ''')
    parser.add_argument('-b', action='store', dest='b', type=str)
    parser.add_argument('-c', action='store', dest='c', type=int)

    print(parser.parse_args())



p_parser = argparse.ArgumentParser(add_help=False)
p_group  = p_parser.add_argument_group('authentication')
p_group.add_argument('--username', dest='user', default=None)
p_group.add_argument('--password', dest='pass', default=None)

p_mutually_exclusive = p_parser.add_mutually_exclusive_group()
p_mutually_exclusive.add_argument('-t', action='store_true')
p_mutually_exclusive.add_argument('-f', action='store_false')



@split_function
def _test_parent_parser():

    parser = argparse.ArgumentParser(parents=[p_parser],)
    parser.add_argument('--local', dest='local', default=None)

    print(parser.parse_args())


@split_function
def _test_conflict_resolver():
    parser = argparse.ArgumentParser(conflict_handler='resolve')

    parser.add_argument('-a', action='store', dest='a')
    parser.add_argument('-b', action='store', dest='b')
    parser.add_argument('-long-b', '-b', action='store', dest='-long-b')

    print(parser.parse_args())


@split_function
def _test_conflict_resolver_v2():
    parser = argparse.ArgumentParser(conflict_handler='resolve')

    parser.add_argument('-a', action='store', dest='a')
    parser.add_argument('-long-b', '-b', action='store', dest='-long-b')
    parser.add_argument('-b', action='store', dest='b')

    print(parser.parse_args())


@split_function
def _test_positional():
    parser = argparse.ArgumentParser(description='this is my positional test')

    parser.add_argument('-a', action='store', dest='a')
    parser.add_argument('-long-b', '-b', action='store', dest='-long-b')
    parser.add_argument('positional', action='store')

    print(parser.parse_args())

@split_function
def _test_group():

    parser = argparse.ArgumentParser(description='this is my positional test',
            parents=[p_parser])

    mygroup = parser.add_argument_group('mygroup')

    mygroup.add_argument('-a', action='store', dest='a')
    mygroup.add_argument('-long-b', '-b', action='store', dest='-long-b')
    mygroup.add_argument('positional', action='store')

    g2 = parser.add_argument_group('GROUP TWO')
    g2.add_argument('-A', action='store', dest='A')
    g2.add_argument('-long-B', '-B', action='store', dest='-long-B')
    g2.add_argument('posB', action='store')


    print(parser.parse_args())


@split_function
def _test_nest_parser():

    parser = argparse.ArgumentParser()

    subparser = parser.add_subparsers(help='commands', dest='action')

    # list 
    list_parser = subparser.add_parser('list', help='List contents')
    list_parser.add_argument('dirname', action='store', help='list directory')

    # create
    create_parser = subparser.add_parser('create', help='Create directory')
    create_parser.add_argument('dirname', action='store', help='create directory')
    create_parser.add_argument('--read-only', action='store_true', default=False, help='create a read only directory')

    # delete
    delete_parser = subparser.add_parser('delete', help='Delete directory')
    delete_parser.add_argument('dirname', action='store', help='delete the specified directory')
    delete_parser.add_argument('-r', '--recursive', action='store_true', default=False, help='delete directory in a recursive way')


    print(parser.parse_args())
    print(parser.get_default(dest='delete'))
    print(parser._get_optional_actions())

    print(parser.parse_args().action, parser.parse_args().dirname)
    #print(dir(parser))




@split_function
def _test_nargs():
    parser = argparse.ArgumentParser()

    parser.add_argument('--three', nargs=3)
    parser.add_argument('--one-or-more', nargs='+')
    parser.add_argument('--any', nargs='*')
    parser.add_argument('--one-or-no', nargs='?')

    print(parser.parse_args())

@split_function
def _test_type_and_mode():
    parser = argparse.ArgumentParser()

    parser.add_argument('-i', type=int)
    parser.add_argument('-f', type=float)
    parser.add_argument('--file', type=open)
    parser.add_argument('--mode', choices=('speed', 'auto', 'normal'), default='normal')

    print(parser.parse_args())

@split_function
def _test_file():
    parser = argparse.ArgumentParser()

    parser.add_argument('-i', metavar='in-file',  type=argparse.FileType('rt'))
    parser.add_argument('-o', metavar='out-file', type=argparse.FileType('wt'))

    print(parser.parse_args())



@split_function
def _test_myown_action():


    class CustomAction(argparse.Action):
        def __init__(self, **kwargs):
            super(CustomAction, self).__init__(**kwargs)

            print('Initializing CustomAction')

            for name,value in kwargs.items():
                if name == 'self' or value is None:
                    continue
                print('\t {} = {!r}'.format(name, value))

            print()

        def __call__(self, parser, namespace, values, option_string=None):
            print('Processing CustomAction for {}'.format(self.dest))
            print('    parser = {}'.format(id(parser)))
            print('    namespace = {}'.format(namespace))
            print('    values = {}'.format(values))
            print('    option_string = {}'.format(option_string))

            if isinstance(values, list):
                values = [v.upper() for v in values]
            else:
                values = values.upper()

            setattr(namespace, self.dest, values)
            print()

                

    parser = argparse.ArgumentParser()

    parser.add_argument('-a', action=CustomAction)
    parser.add_argument('-m', nargs='*', action=CustomAction)

    print(parser.parse_args())



if __name__ == '__main__':
    print("hello, test argparse " + str(sys.argv))
    #_test()
    #_test_long_args()
    #_test_non_minus()
    #_test_store()
    #_test_prefix()
    #_test_config_parser()
    #_test_config_parser_v2()
    #_test_helper_disabled()
    #_test_helper()
    #_test_raw_description_help_formatter()
    #_test_raw_text_help_formatter()
    #_test_metavar_type_help_formatter()
    #_test_parent_parser()
    #_test_conflict_resolver()
    #_test_conflict_resolver_v2()
    #_test_positional()
    #_test_group()
    _test_nest_parser()
    #_test_nargs()
    #_test_type_and_mode()
    #_test_file()
    #_test_myown_action()



