import threading


def len_i(i):
    return sum(1 for e in i)


class LockVar():
    def __init__(self, num):
        self.lock = threading.Lock()
        self.num = num

    def get(self):
        self.lock.acquire()
        num = self.num
        self.lock.release()
        return num

    def set(self, num):
        self.lock.acquire()
        self.num = num
        self.lock.release()
