import sys
from . import log
from . import web_server
log.DEBUG = False
web_server.SERVER_DOMAIN = None


class Resources:
    def __init__(self):
        self.level_1_conn = None
        self.scheduler = None
        self.mod_manager = None


def main():
    import getopt
    opts, args = getopt.getopt(sys.argv[1:], 'dp:t:s:a:o:', ["level0port=", "http-domain=", "http-port=", "https-port=", "audit-path="])
    level0_port = 5000
    http_port = 8080
    https_port = 0
    for o, a in opts:
        if o in ['-p', '--level0port']:
            level0_port = int(a)
        elif o in ['-o', '--http-domain']:
            from . import web_server
            web_server.SERVER_DOMAIN = str(a)
        elif o in ['-t', '--http-port']:
            http_port = int(a)
        elif o in ['-s', '--https-port']:
            https_port = int(a)
        elif o in ['-a', '--audit-path']:
            log.OUTPUT_FD = open(a, 'a')
        elif o in ['-d']:
            log.DEBUG = True
        else:
            print('unrecognizable argument: ' + str(a))
            sys.exit(1)

    res = Resources()
    from .level1_connector import Level1Connector
    res.level_1_conn = Level1Connector(res, level0_port)
    from .scheduler import Scheduler
    res.scheduler = Scheduler(res)
    from .module_manager import ModuleManager
    res.mod_manager = ModuleManager(res)
    from . import web_server
    web_server.server_setup(res, http_port, https_port)
    from .control import control_loop
    control_loop(res)
