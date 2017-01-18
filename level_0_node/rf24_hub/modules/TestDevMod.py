import json
import os
import time
import traceback

import tornado.web
from tornado import gen
from tornado.concurrent import Future

from ..log import *
from .base_module import BaseModule


class TestDevMod(BaseModule):
    def __init__(self, res, uuid):
        super().__init__(res, uuid)
        self.waiting = dict()
        self.light_on = False

    def get_rf24_listen_addr(self):
        return [20]

    def handle_new_rf24_message(self, packet_type, scr_addr, packet_content):
        if len(packet_content) is not 5:
            warn('TestDevMod receive abnormal length rf24 packet')
            return
        event_id = packet_content[0:4]
        if event_id in self.waiting:
            try:
                if packet_content[4] == 0x00:
                    self.waiting[event_id][1].set_result(json.dumps({'result': 'success'}))
                elif packet_content[4] == 0x02:
                    self.waiting[event_id][1].set_result(json.dumps({'result': 'no change'}))
                else:
                    self.waiting[event_id][1].set_result(json.dumps({'result': 'unknown error'}))
            except ValueError and TypeError:
                debug(traceback.format_exc())
                self.waiting[event_id][1].set_result(json.dumps({'result': 'unknown command'}))
                return
        else:
            warn('event_id not found')

    def clear_waiting(self, event_id):
        if event_id in self.waiting:
            self.waiting[event_id][1].set_result(json.dumps({'result': 'command timeout'}))

    def on_request(self, event_id):
        return self.send_rf24_packet(1, 20, bytes(event_id + bytes([0x01, 0x01])))

    def off_request(self, event_id):
        return self.send_rf24_packet(1, 20, bytes(event_id + bytes([0x01, 0x02])))

    def web_gui_generate(self, conn_handler, username):
        if self.is_exist_in_network(20):
            online_str = 'online'
        else:
            online_str = 'offline'
        if self.light_on:
            status_str = 'on'
        else:
            status_str = 'off'
        return conn_handler.render_string('TestDevMod.html', uuid=str(self.uuid), online_str=online_str, status_str=status_str)

    def get_ajax_handler(self):
        mod = self

        class PostHandler(tornado.web.RequestHandler):
            @gen.coroutine
            def post(self):
                my_future = Future()
                self.event_id = os.urandom(4)
                try:
                    command = json.loads(self.request.body.decode('utf-8'))
                    if command['command'] == 'on':
                        if not mod.on_request(self.event_id):
                            self.write('{"result":"device not found"}')
                            return
                    elif command['command'] == 'off':
                        if not mod.off_request(self.event_id):
                            self.write('{"result":"device not found"}')
                            return
                    else:
                        raise ValueError
                except ValueError and TypeError:
                    debug(traceback.format_exc())
                    self.write('{"result":"format error"}')
                    return
                mod.waiting[self.event_id] = (command, my_future)
                mod.schedule_to_run(time.time() + 5, mod.clear_waiting, self.event_id)
                result = yield my_future
                del mod.waiting[self.event_id]
                if not self.request.connection.stream.closed():
                    self.write(result)
                return

            def on_connection_close(self):
                mod.clear_waiting(self.event_id)

        return PostHandler
