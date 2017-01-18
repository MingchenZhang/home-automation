import sys

from tornado import ioloop

from . import log
from . import level0_connector
from . import camera


class Resources:
    def __init__(self):
        self.level_0_conn = None
        self.camera_module = None


def main():
    import getopt
    opts, args = getopt.getopt(sys.argv[1:], 's:p:a:l:d',
                               ['level0server=', 'level0port=', 'audit-path=', 'local-device-no='])
    level0_addr = 'localhost'
    level0_port = 5000
    local_device_no = None
    log.DEBUG = False
    for o, a in opts:
        if o in ['-s', '--level0server']:
            level0_addr = str(a)
        elif o in ['-p', '--level0port']:
            level0_port = int(a)
        elif o in ['-a', '--audit-path']:
            log.OUTPUT_FD = open(a, 'a')
        elif o in ['-d']:
            log.DEBUG = True
        elif o in ['-l', '--local-device-no']:
            local_device_no = int(a)
        else:
            print('unrecognizable argument: ' + str(a))

    if local_device_no is None:
        print('device No. has to be given in argument via `-l`(`--local-device-no`) argument')
        sys.exit(1)

    res = Resources()
    res.level_0_conn = level0_connector.Level0Connector(res, level0_addr, level0_port, local_device_no)
    res.camera_module = camera.CameraModule()
    ioloop.IOLoop.current().start()
