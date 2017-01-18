import base64
import json
import traceback
import time

import tornado.web
from tornado import gen
from tornado.concurrent import Future

from . import base_module
from ..log import *

STILL_IMAGE_EVENT_KEY = 'send-back.camera.current-image'


class Camera(base_module.BaseModule):
    def __init__(self, res, uuid):
        super().__init__(res, uuid)
        self.unit_no = 23
        self.last_image = None
        self.last_image_waiting_list = {}
        self.subscribe_event(STILL_IMAGE_EVENT_KEY, self.accept_new_image)
        pass

    def accept_new_image(self, subscriber_data, event_data):
        image_bytes = base64.b64decode(event_data)
        self.last_image = image_bytes
        for future_to_fulfill in self.last_image_waiting_list:
            future_to_fulfill.set_result(True)
            pass
        pass

    def request_still_image(self, resolution=None, quality=None, video_port=None):
        # {"type":"still_image_request", "resolution":[1296, 972], "video_port":True}
        resolution = resolution or (1296, 972)
        quality = quality or 85
        video_port = (video_port if video_port is not None else True)
        request = {"type": "still_image_request", "resolution": list(resolution), "quality": int(quality),
                   "video_port": video_port, "send_back_event_key": STILL_IMAGE_EVENT_KEY}
        self.send_json_to_device(self.unit_no, request)
        pass

    def request_continuous_image(self, resolution=None, quality=None, fps=None, video_port=None):
        # {"type":"still_continuous_image_request", "resolution":[1296, 972], "fps":15, "video_port":True}
        resolution = resolution or (1296, 972)
        quality = quality or 85
        fps = fps or 15
        video_port = (video_port if video_port is not None else True)
        request = {"type": "still_continuous_image_request", "resolution": list(resolution), "quality": int(quality),
                   "fps": fps, "video_port": video_port, "send_back_event_key": STILL_IMAGE_EVENT_KEY}
        self.send_json_to_device(self.unit_no, request)
        pass

    def clear_last_image_waiting_list(self, future_to_fulfill):
        if future_to_fulfill in self.last_image_waiting_list:
            future_to_fulfill.set_result(False)

    def get_rf24_listen_addr(self):
        return []

    def handle_new_rf24_message(self, packet_type, scr_addr, packet_content):
        pass

    def web_gui_generate(self, conn_handler, username):
        if self.is_exist_in_network(self.unit_no):
            online_str = 'online'
        else:
            online_str = 'offline'
        if username is not None:
            display_control = True
        else:
            display_control = False

        return conn_handler.render_string('Camera.html',
                                          uuid=str(self.uuid),
                                          online_str=online_str,
                                          display_control=display_control)

    def get_ajax_handler(self):
        mod = self

        class AjaxHandler(tornado.web.RequestHandler):
            def prepare(self):
                debug('new http request from:' + self.request.uri)
                debug('content:' + self.request.body.decode('utf-8'))

            def get(self, path):
                if path == '/current_image':
                    # first thing first, permission check
                    username_b = self.get_secure_cookie('username')
                    if username_b is None:
                        self.set_status(401)
                        self.finish()
                        return
                    # TODO: simplify above identification procedure
                    if mod.last_image is not None:
                        self.set_header('Content-Type', 'image/jpeg')
                        self.write(mod.last_image)
                        self.finish()
                    else:
                        self.set_status(404)
                        self.finish()
                else:
                    self.set_status(404)
                    self.finish()
                return

            @gen.coroutine
            def post(self, path):
                self.my_future = Future()
                try:
                    command = json.loads(self.request.body.decode('utf-8'))
                    if not mod.is_exist_in_network(mod.unit_no):
                        self.write('{"result":"device not found"}')
                        self.finish()
                        return
                    if command['request'] == 'still_image':
                        mod.request_still_image(resolution=command.get('resolution', None),
                                                quality=command.get('quality', None),
                                                video_port=command.get('video_port', None))
                        mod.last_image_waiting_list[self.my_future] = True
                        mod.schedule_to_run(time.time() + 9, mod.clear_last_image_waiting_list, self.my_future)
                        if (yield self.my_future):
                            result = '{"result":"success"}'
                        else:
                            result = '{"result":"timeout"}'
                        del mod.last_image_waiting_list[self.my_future]
                        if not self.request.connection.stream.closed():
                            self.write(result)
                    elif command['request'] == 'still_continuous_image':
                        mod.request_continuous_image(resolution=command.get('resolution', None),
                                                     quality=command.get('quality', None),
                                                     fps=command.get('fps', None),
                                                     video_port=command.get('video_port', None))
                        mod.last_image_waiting_list[self.my_future] = True
                        mod.schedule_to_run(time.time() + 9, mod.clear_last_image_waiting_list, self.my_future)
                        if (yield self.my_future):
                            result = '{"result":"success"}'
                        else:
                            result = '{"result":"timeout"}'
                        del mod.last_image_waiting_list[self.my_future]
                        if not self.request.connection.stream.closed():
                            self.write(result)
                    else:
                        raise ValueError()
                except (ValueError, TypeError, LookupError):
                    debug(traceback.format_exc())
                    self.write('{"result":"format error"}')
                    self.finish()
                    return
                self.finish()
                return

            def on_connection_close(self):
                if hasattr(self, 'my_future'):
                    mod.clear_last_image_waiting_list(self.my_future)

        return AjaxHandler
