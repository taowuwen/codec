from queue import PriorityQueue

class FilePriorityQueue(PriorityQueue):
    def __init__(self, name = 'filequeue', *args, **kwargs):

        self._name = name
        super().__init__(*args, **kwargs)

    def put_level1(self, msg):
        self.put((0, msg))

    def put_level2(self, msg):
        self.put((1, msg))

    def put_level3(self, msg):
        self.put((2, msg))

    put_msg = put_level3

    def put_level4(self, msg):
        self.put((3, msg))

    def put_level5(self, msg):
        self.put((4, msg))

    @property
    def name(self):
        return self._name

    def get_msg(self):
        return self.get()[1]

if __name__ == '__main__':
    q = FilePriorityQueue('tmp')

    print('queue name ', q.name)
    q.put_msg('hello, world')
    q.put_level1('level 1 a')
    q.put_level2('level 2 a')
    q.put_level3('level 3 a')
    q.put_level4('level 4 a')
    q.put_level3('level 3 b')
    q.put_level4('level 4 b')
    q.put_level5('level 5 a')
    q.put_level5('level 5 b')
    q.put_level1('level 1 b')
    q.put_level1('level 1 c')

    while not q.empty():
        print(q.get_msg())

