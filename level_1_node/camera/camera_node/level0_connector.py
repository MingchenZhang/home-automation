import base64
import json
import socket as so
import threading

import io
import traceback

import time
from tornado import ioloop
from .log import *


class Level0Connector:
    def __init__(self, res, server_name, server_port, device_no):
        self.res = res
        self.device_no = device_no
        self.server_conn = None
        self.server_conn_addr = (server_name, server_port)
        self.server_conn_buffer = bytearray()
        self.server_write_lock = threading.Lock()
        self.reconnect()
        pass

    def reconnect(self):
        try:
            if self.server_conn is not None:
                return
            self.server_conn = so.socket(so.AF_INET, so.SOCK_STREAM)
            self.server_conn.connect(self.server_conn_addr)
            self.server_conn.setsockopt(so.IPPROTO_TCP, so.TCP_NODELAY, 1)
            succ('successfully connected to server: ' + str(self.server_conn_addr))
            ioloop.IOLoop.current().add_handler(self.server_conn, self.new_message, ioloop.IOLoop.READ)
        except ConnectionError:
            debug(traceback.format_exc())
            warn('reconnection failed')
            if self.server_conn is not None:
                self.server_conn.close()
                self.server_conn = None
            ioloop.IOLoop.current().add_timeout(datetime.timedelta(seconds=5), self.reconnect)

    def send_device_list(self, *args, **kwargs):
        if self.server_conn is not None:
            device_list = {"type": "device_list", "device_list": [{"address": int(0xaaaaaaaa00 + self.device_no)}]}
            with self.server_write_lock:
                try:
                    self.server_conn.sendall(json.dumps(device_list).encode('ascii')+b'\n')
                except ConnectionError:
                    debug(traceback.format_exc())
                    warn('fail to send send_device_list')

    def new_message(self, socket, event):
        try:
            new_data = self.server_conn.recv(1024)
        except ConnectionError:
            new_data = None
        if not new_data:
            warn('server ' + str(self.server_conn_addr) + ' disconnected')
            ioloop.IOLoop.current().remove_handler(self.server_conn)
            with self.server_write_lock:
                self.server_conn.close()
                self.server_conn = None
            ioloop.IOLoop.current().add_timeout(datetime.timedelta(seconds=5), self.reconnect)
            return
        self.server_conn_buffer += new_data
        while True:
            newline_index = self.server_conn_buffer.find(bytes([0x0A]))  # find new line
            if newline_index != -1:
                newline = self.server_conn_buffer[:newline_index]
                self.server_conn_buffer = self.server_conn_buffer[newline_index + 1:]
                try:
                    newline_str = newline.decode('utf-8')
                except ValueError:
                    warn('New message from ' + str(self.server_conn_addr) + ' cannot be processed')
                    continue
                debug('New message received from ' + str(self.server_conn_addr) + ':')
                debug(newline_str)
                self.process_message(newline_str)
            else:
                break

    def process_message(self, newline):
        try:
            message = json.loads(newline)  # TODO_: might raise ValueError and TypeError
        except ValueError and TypeError:
            warn('New message from ' + str(self.server_conn_addr) + ' cannot be decoded')
            return

        def device_list_get():
            self.send_device_list()

        def still_image():
            try:
                resolution = message.get('resolution', [1296, 972])
                resolution = (int(resolution[0]), int(resolution[1]))
                quality = int(message.get('quality', 85))
                video_port = message.get('video_port', True)
                event_str = str(message['send_back_event_key'])
            except (ValueError, TypeError):
                debug(traceback.format_exc())
                warn('server send unrecognizable message: ' + newline)
                return
            try:
                stream = self.res.camera_module.capture_still_image(resolution, quality, video_port=video_port)
                if stream is not None:
                    reply = {"type": "event", "event_key": event_str, "data": base64.b64encode(stream.getvalue()).decode('ascii')}
                    with self.server_write_lock:
                        self.server_conn.sendall(json.dumps(reply).encode('ascii')+b'\n')
                else:
                    return
            except Exception:
                debug(traceback.format_exc())
                error('image generating/sending failed')
                return

        def still_continuous_image():
            try:
                resolution = message.get('resolution',[1296, 972])
                resolution = (int(resolution[0]), int(resolution[1]))
                quality = int(message.get('quality', 85))
                fps = int(message.get('fps', 15))
                video_port = message.get('video_port', True)
                event_str = str(message['send_back_event_key'])
            except (ValueError, TypeError):
                debug(traceback.format_exc())
                warn('server send unrecognizable message: ' + newline)
                return

            def callback(stream):
                try:
                    b64 = base64.b64encode(stream.getvalue())
                    reply = {"type": "event", "event_key": event_str,
                             "data": b64.decode('ascii')}
                    with self.server_write_lock:
                        start_time = time.time()
                        self.server_conn.sendall(json.dumps(reply).encode('ascii')+b'\n')
                        debug('test cost '+str(time.time()-start_time)+' second')
                except Exception:
                    debug(traceback.format_exc())
                    error('image generating/sending failed')
                    return False
                return True

            self.res.camera_module.capture_continuous_images(callback, resolution, quality, fps, video_port=video_port)
            return

        def invalid_message():
            pass

        debug('request received:'+str(message))
        if 'type' in message:
            {'device_list_get': device_list_get,
             'still_image_request': still_image,
             'still_continuous_image_request': still_continuous_image,
             }.get(message['type'], invalid_message)()
        return
