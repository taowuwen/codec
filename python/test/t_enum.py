#!/usr/bin/env python3


import enum
from functools import wraps


class BugStatus(enum.Enum):
    new = 7
    incomplete = 6
    invalid = 5
    wont_fix = 4
    in_progress = 3
    fix_complete = 2
    fix_released = 1


class BugStatusInt(enum.IntEnum):
    new = 7
    incomplete = 6
    invalid = 5
    wont_fix = 4
    in_progress = 3
    fix_complete = 2
    fix_released = 1


class BugStatusRep(enum.Enum):
    new = 7
    incomplete = 6
    invalid = 5
    wont_fix = 4
    in_progress = 3
    fix_complete = 2
    fix_released = 1

    by_design = 4
    closed = 1


@enum.unique
class BugStatusRepUniq(enum.Enum):
    new = 7
    incomplete = 6
    invalid = 5
    wont_fix = 4
    in_progress = 3
    fix_complete = 2
    fix_released = 1

   # by_design = 4
   # closed = 1

def next_line(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        print("\n--------{:^20}--------------".format(func.__name__))
        return func(*args, **kwargs)

    return wrapper


@next_line
def test_one():
    print("status {}".format(BugStatus.wont_fix))
    print("status name {}".format(BugStatus.wont_fix.name))
    print("status value {}".format(BugStatus.wont_fix.value))


    for item in BugStatus:
        print("{:30}: {:15} = {:5}".format(item, item.name, item.value))

@next_line
def test_two():
    actual_state = BugStatus.wont_fix
    desired_state = BugStatus.fix_released


    print('equality == ', 
          actual_state == desired_state,
          actual_state == BugStatus.wont_fix)

    print('identify "is"',
          actual_state is desired_state,
          actual_state is BugStatus.wont_fix)

    try:
        print("\n".join(s.name for s in sorted(BugStatus)))

    except Exception as e:
        print(e)

    try:
        print("\n".join("\t" + s.name for s in sorted(BugStatusInt)))

    except Exception as e:
        print(e)

@next_line
def test_three():

    for item in BugStatusRep:
        print("{:<30}> {:>15} = {:<5}".format(item, item.name, item.value))

    print("BugStatusRep.wont_fix == BugStatusRep.by_design ?", 
          BugStatusRep.wont_fix == BugStatusRep.by_design)

    print("BugStatusRep.wont_fix is BugStatusRep.by_design ?", 
          BugStatusRep.wont_fix is BugStatusRep.by_design)


@next_line
def test_five():
    BugStatus = enum.Enum(
        value="BugStatus",
        names=("new incomplete invalid wont_fix in_progress "
              "fix_complete fix_released")
    )

    print("status {}".format(BugStatus.wont_fix))
    print("status name {}".format(BugStatus.wont_fix.name))
    print("status value {}".format(BugStatus.wont_fix.value))

    for item in BugStatus:
        print("{:<30}> {:>15} = {:<5}".format(item, item.name, item.value))


@next_line
def test_six():
    BugStatus = enum.Enum(
        value='BugStatus',
        names=[('new', 7),
               ('incomplete', 6),
               ('wont_fix', 4),
              ]
    )

    print("status {}".format(BugStatus.wont_fix))
    print("status name {}".format(BugStatus.wont_fix.name))
    print("status value {}".format(BugStatus.wont_fix.value))

    for item in BugStatus:
        print("{:<30}> {:>15} = {:<5}".format(item, item.name, item.value))




class BugStatusNonInteger(enum.Enum):
    new = (7, ['incomplete',
               'invalid',
               'wont_fix',
               'in_progress'])
    incomplete = (6, ['new', 'wont_fix'])
    invalid = (5, ['new'])
    wont_fix = (4, ['new'])
    in_progress = (3, ['new', 'fix_committed'])
    fix_committed = (2, ['in_progress', 'fix_released'])
    fix_released = (1, ['new'])

    def __init__(self, num, transitions):
        self.num = num
        self.transitions = transitions

    def can_transition(self, new_state):
        return new_state.name in self.transitions


class BugStatusNonIntegerDict(enum.Enum):
    new = {
        "num":7,
        "value": [
            "incomplete",
            "invalid",
            "wont_fix",
            "in_progress"
            ]
    }

    incomplete = {
        "num": 6,
        "value": ["new", "wont_fix"]
    }

    invalid = {
        "num": 5,
        "value" : ["new"]
    }

    wont_fix = {
        "num": 4,
        "value": ["new"]
    }

    in_progress = {
        "num": 3,
        "value": ["new", "fix_committed"]
    }

    fix_committed = {
        "num": 2,
        "value": ["in_progress", "fix_released"]
    }

    fix_released = {
        "num": 1,
        "value": ["new"]
    }

    def __init__(self, vals):
        self.num = vals['num']
        self.transitions = vals['value']

    def can_transition(self, new_state):
        return new_state.name in self.transitions


@next_line
def test_seven():
    BugStatus = BugStatusNonInteger

    print("status {}".format(BugStatus.wont_fix))
    print("status name {}".format(BugStatus.wont_fix.name))
    print("status value {}".format(BugStatus.wont_fix.value))
    print("status can_transition {}".format(BugStatus.wont_fix.can_transition(BugStatus.new)))

    for item in BugStatus:
        print("{:<40}> {:>15} = {}".format(item, item.name, item.value))



@next_line
def test_eight():
    BugStatus = BugStatusNonIntegerDict

    print("status {}".format(BugStatus.wont_fix))
    print("status name {}".format(BugStatus.wont_fix.name))
    print("status value {}".format(BugStatus.wont_fix.value))
    print("status can_transition {}".format(BugStatus.wont_fix.can_transition(BugStatus.new)))

    for item in BugStatus:
        print("{:<40}> {:>15} = {}".format(item, item.name, item.value))


if __name__ == '__main__':
    print("hello, test for enum")
    test_one()
    test_two()
    test_three()
    # test_four()
    test_five()
    test_six()
    test_seven()
    test_eight()
