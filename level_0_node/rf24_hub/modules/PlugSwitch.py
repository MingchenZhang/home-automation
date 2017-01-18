import json
import os
import time
import traceback

import tornado.web
from tornado import gen
from tornado.concurrent import Future

from ..log import *
from .base_module import BaseModule


class PlugSwitch(BaseModule):
    def __init__(self, res, uuid):
        super().__init__(res, uuid)
        self.waiting = dict()
        self.switch_1_on = False
        self.switch_2_on = False
        self.unit_no = 21

    def get_rf24_listen_addr(self):
        return [self.unit_no]

    def handle_new_rf24_message(self, packet_type, scr_addr, packet_content):
        # if len(packet_content) is not 5:
        #     warn('TestDevMod receive abnormal length rf24 packet')
        #     return
        if packet_type is 2:
            event_id = packet_content[0:4]
            if event_id in self.waiting and self.waiting[event_id][2]:
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
        # elif packet_type is 4 and packet_content[0:] == bytes([0, 0, 0, 0, 0, 0, 0xFF]):
        #     for event_id in self.waiting:
        #         try:
        #             if self.waiting[event_id][0]['command'] == 'on':
        #                 self.on_request(event_id, self.waiting[event_id][0]['target'])
        #                 self.waiting[event_id][2] = True
        #             else:
        #                 self.waiting[event_id][1].set_result(json.dumps({'result': 'unknown error'}))
        #         except ValueError and TypeError:
        #             debug(traceback.format_exc())
        #             self.waiting[event_id][1].set_result(json.dumps({'result': 'unknown command'}))
        #             return
        elif packet_type is 4:
            if packet_content[4] == 1:
                if packet_content[6] == 0:
                    self.switch_1_on = False
                elif packet_content[6] == 1:
                    self.switch_1_on = True
                else:
                    self.switch_1_on = None
            elif packet_content[4] == 2:
                if packet_content[6] == 0:
                    self.switch_2_on = False
                elif packet_content[6] == 1:
                    self.switch_2_on = True
                else:
                    self.switch_2_on = None
        else:
            warn('packet_type didn\'t match recoed')

    def clear_waiting(self, event_id):
        if event_id in self.waiting:
            self.waiting[event_id][1].set_result(json.dumps({'result': 'command timeout'}))

    def send_request(self):
        for event_id in self.waiting:
            try:
                if not self.waiting[event_id][2]:  # not yet send
                    if self.waiting[event_id][0]['command'] == 'on':
                        self.on_request(event_id, self.waiting[event_id][0]['target'])
                        self.waiting[event_id][2] = True
                    elif self.waiting[event_id][0]['command'] == 'off':
                        self.off_request(event_id, self.waiting[event_id][0]['target'])
                        self.waiting[event_id][2] = True
                    else:
                        self.waiting[event_id][1].set_result(json.dumps({'result': 'unknown error'}))
            except ValueError and TypeError:
                debug(traceback.format_exc())
                self.waiting[event_id][1].set_result(json.dumps({'result': 'unknown command'}))
                return

    def on_request(self, event_id, target):
        return self.send_rf24_packet(1, self.unit_no,
                                     bytes(event_id + bytes([target, 1])))  # TODO: send trigger period

    def off_request(self, event_id, target):
        return self.send_rf24_packet(1, self.unit_no,
                                     bytes(event_id + bytes([target, 2])))  # TODO: send trigger period

    def web_gui_generate(self, conn_handler, username):
        if self.is_exist_in_network(self.unit_no):
            online_str = 'online'
        else:
            online_str = 'offline'
        if self.switch_1_on:
            status_1_str = 'on'
        elif self.switch_1_on == False:
            status_1_str = 'off'
        else:
            status_2_str = 'unknown'
        if self.switch_2_on:
            status_2_str = 'on'
        elif self.switch_2_on == False:
            status_2_str = 'off'
        else:
            status_1_str = 'unknown'

        if username is not None:
            display_control = True
        else:
            display_control = False

        return conn_handler.render_string('PlugSwitch.html',
                                          uuid=str(self.uuid),
                                          online_str=online_str,
                                          status_1_str=status_1_str,
                                          status_2_str=status_2_str,
                                          display_control=display_control)

    def get_ajax_handler(self):
        mod = self

        class PostHandler(tornado.web.RequestHandler):
            def prepare(self):
                debug('new http request from:' + self.request.uri)
                debug('content:' + self.request.body.decode('utf-8'))

            @gen.coroutine
            def post(self, path):
                # first thing first, permission check
                username_b = self.get_secure_cookie('username')
                if username_b is None:
                    self.set_status(401)
                    self.finish()
                    return
                # TODO: simplify above identification procedure
                my_future = Future()
                while True:
                    self.event_id = os.urandom(4)
                    if self.event_id != bytes([0,0,0,0]):
                        break
                try:
                    command = json.loads(self.request.body.decode('utf-8'))
                    if not mod.is_exist_in_network(mod.unit_no):
                        self.write('{"result":"device not found"}')
                        self.finish()
                        return
                except (ValueError, TypeError, LookupError):
                    debug(traceback.format_exc())
                    self.write('{"result":"format error"}')
                    self.finish()
                    return
                mod.waiting[self.event_id] = [command, my_future, False]
                mod.send_request()
                mod.schedule_to_run(time.time() + 9, mod.clear_waiting, self.event_id)
                result = yield my_future
                del mod.waiting[self.event_id]
                if not self.request.connection.stream.closed():
                    self.write(result)
                self.finish()
                return

            def on_connection_close(self):
                mod.clear_waiting(self.event_id)

        return PostHandler
