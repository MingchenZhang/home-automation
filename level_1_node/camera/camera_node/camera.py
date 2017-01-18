import base64
import threading
import time

import io

import picamera

from camera_node import tools
from .log import *


class CameraModule:
    def __init__(self):
        self.continuous_timer = -1
        self.continuous_timer_lock = threading.Lock()
        self.camera_lock = threading.Lock()
        pass

    def capture_still_image(self, resolution, quality, video_port=True):
        info('still image requested')
        if self.camera_lock.acquire(blocking=False):
            try:
                stream = io.BytesIO()  # TODO: check all project stream close
                with picamera.PiCamera() as camera:
                    camera.resolution = resolution
                    camera.rotation = 180
                    time.sleep(2)
                    camera.capture(stream, 'jpeg', quality=quality, use_video_port=video_port)
                succ('still image captured')
                return stream
            finally:
                self.camera_lock.release()
        else:
            return None

    def capture_continuous_images(self, stream_callback, resolution, quality, fps, video_port=True):
        info('still continuous image requested')

        def stream_generator():
            stream = io.BytesIO()
            while True:
                with self.continuous_timer_lock:
                    if time.time() > self.continuous_timer:
                        self.continuous_timer = -1
                        return
                    debug('time remaining:'+str(self.continuous_timer - time.time()))
                yield stream
                succ('still continuous image captured')
                stream.seek(0)
                if not stream_callback(stream):
                    return
                stream.seek(0)
                stream.truncate()

        cam = self

        class CaptureThread(threading.Thread):
            def __init__(self, streams, resolution, quality, fps, video_port=True):
                super().__init__()
                self.streams = streams
                self.resolution = resolution
                self.quality = quality
                self.fps = fps
                self.video_port = video_port

            def run(self):
                with cam.camera_lock:
                    with picamera.PiCamera() as camera:
                        camera.resolution = self.resolution
                        camera.framerate = self.fps
                        camera.rotation = 180
                        time.sleep(2)
                        camera.capture_sequence(self.streams, 'jpeg', quality=self.quality,
                                                use_video_port=self.video_port)

        with self.continuous_timer_lock:
            if self.continuous_timer == -1:
                spawn_needed = True
            else:
                spawn_needed = False
            self.continuous_timer = time.time() + 30

        # TODO: camera configuration change detection
        if spawn_needed:
            cap_thread = CaptureThread(stream_generator(), resolution, quality, fps, video_port)
            cap_thread.start()
        return
