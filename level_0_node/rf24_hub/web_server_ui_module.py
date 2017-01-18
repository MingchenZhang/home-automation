import os

import tornado.web


# generate control_panel UI MODULE
class ControlPanelUICSS(tornado.web.UIModule):
    def render(self):
        return b'<style>' + self.render_string(os.path.join("static", 'bootstrap-3.3.6', 'css', 'bootstrap.min.css')) + b'</style>' + \
               b'<style>' + self.render_string(os.path.join("static", 'control_panel.css')) + b'</style>'


class ControlPanelUIJS(tornado.web.UIModule):
    def render(self):
        return b'<script>' + self.render_string(os.path.join("static", 'jquery-2.2.4.min.js')) + b'</script>' + \
               b'<script>' + self.render_string(os.path.join("static", 'bootstrap-3.3.6', 'js', 'bootstrap.min.js')) + b'</script>' + \
               b'<script>' + self.render_string(os.path.join("static", 'jquery.bootstrap-growl.min.js')) + b'</script>'


class CommonButtonHandlerUI(tornado.web.UIModule):
    def render(self):
        return b'<script>' + self.render_string(os.path.join("static", 'common_button_handler.js')) + b'</script>'


class CameraModuleUI(tornado.web.UIModule):
    def render(self):
        return b'<script>' + self.render_string(os.path.join("static", 'camera_module.js')) + b'</script>'