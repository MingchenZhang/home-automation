from heapq import *


class Scheduler:
    def __init__(self, res):
        self.res = res
        self.pending_schedule = []

    def add_schedule(self, subscriber, time, callback, subscriber_data=None):
        heappush(self.pending_schedule, (time, callback, subscriber_data, subscriber))

    def check_schedule(self):
        import time
        while True:
            if len(self.pending_schedule) > 0 and self.pending_schedule[0][0] <= time.time():
                event = heappop(self.pending_schedule)
                self.res.mod_manager.module_lock(event[3])
                if event[2] is not None:
                    event[1](event[2])
                else:
                    event[1]()
                self.res.mod_manager.module_unlock(event[3])
            else:
                break
