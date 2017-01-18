import time
from tornado import ioloop

last_check_count = 0


def timer_check(res):
    global last_check_count
    res.scheduler.check_schedule()
    if last_check_count % 20 == 0:  # TODO: decrease request interval
        res.level_1_conn.request_refresh_clients()
    last_check_count += 1
    ioloop.IOLoop.current().call_later(1, timer_check, res=res)


def control_loop(res):
    ioloop.IOLoop.current().call_later(1, timer_check, res=res)
    ioloop.IOLoop.current().start()
    
    
            
    # while True:
    #     events = res.selector.select(timeout=1)
    #     for key, mask in events:
    #         callback = key.data
    #         callback(key.fileobj)
    #     res.scheduler.check_schedule()
    #     # TODO_: periodically refresh device list under each level1 device
    #     if time.time() - last_device_list_request > 20:  # TODO: decrease request interval
    #         last_device_list_request = time.time()
    #         res.level_1_conn.request_refresh_clients()
    #     continue






