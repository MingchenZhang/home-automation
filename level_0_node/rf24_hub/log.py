import datetime

DEBUG = False
OUTPUT_FD = None


def p(line):
    out = line
    if not OUTPUT_FD:
        print(out)
    else:
        OUTPUT_FD.write(out + '\n')
        OUTPUT_FD.flush()


def debug(line):
    if DEBUG:
        out = '\033[35m' + 'DEBUG(' + str(datetime.datetime.now()) + '): ' + '\033[0m' + str(line)[:500]
        if not OUTPUT_FD:
            print(out)
        else:
            OUTPUT_FD.write(out + '\n')
            OUTPUT_FD.flush()



def warn(line):
    out = '\033[33m' + 'WARN(' + str(datetime.datetime.now()) + '): ' + '\033[0m' + str(line)[:200]
    if not OUTPUT_FD:
        print(out)
    else:
        OUTPUT_FD.write(out + '\n')
        OUTPUT_FD.flush()


def error(line):
    out = '\033[31m' + 'ERROR(' + str(datetime.datetime.now()) + '): ' + '\033[0m' + str(line)[:200]
    if not OUTPUT_FD:
        print(out)
    else:
        OUTPUT_FD.write(out + '\n')
        OUTPUT_FD.flush()


def succ(line):
    out = '\033[32m' + 'SUCCESS(' + str(datetime.datetime.now()) + '): ' + '\033[0m' + str(line)[:200]
    if not OUTPUT_FD:
        print(out)
    else:
        OUTPUT_FD.write(out + '\n')
        OUTPUT_FD.flush()


def info(line):
    out = '\033[34m' + 'INFO(' + str(datetime.datetime.now()) + '): ' + '\033[0m' + str(line)[:200]
    if not OUTPUT_FD:
        print(out)
    else:
        OUTPUT_FD.write(out + '\n')
        OUTPUT_FD.flush()
