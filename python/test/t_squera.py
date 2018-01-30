#!/usr/bin/env python3


# +-----+-----+
# |     |     |
# |     |     |
# |     |     |
# |     |     |
# +-----+-----+
# |     |     |
# |     |     |
# |     |     |
# |     |     |
# +-----+-----+

def _test_1():

    def get_ind(n):
        return 1 if n % 5 else 0


    def get_char(row, col):
        sym = [('+', '-'), ('|', ' ')]

        return sym[get_ind(row)][get_ind(col)]

    for row in range(11):
        for col in range(11):
            ch = get_char(row, col)
            print(ch, end='')

        print()


if __name__ == '__main__':
    _test_1()
