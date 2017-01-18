import hashlib
import json
import os.path
import sqlite3
import ssl

import tornado.web
import tornado.httpserver
import tornado.ioloop
from .tools import len_i
from .log import *
from . import web_server_ui_module

# by defining SERVER_DOMAIN global variable, the server will use certificate under /etc/letsencrypt/live/$domain
SERVER_DOMAIN = None


def server_setup(res, http_server_port, https_server_port):  # TODO: change to an object
    def create_user(username, raw_password, user_level):
        if not isinstance(user_level, int) or not 0 <= user_level <= 10:
            raise TypeError('user_level should be an integer between 0~10 (inclusive)')
        salt = (''.join(format(x, '02x') for x in os.urandom(64)))
        salted_pass = raw_password + salt
        stored_pass = hashlib.sha256(salted_pass.encode('utf-8')).hexdigest()
        user_db.execute("INSERT INTO user_info (user_name,user_pass,user_pass_salt,user_level) VALUES ("
                        "'" + username + "'," +
                        "'" + stored_pass + "'," +
                        "'" + salt + "'," +
                        str(user_level) +
                        ");")  # TODO: arguments validity check
        user_db.commit()
        return

    def change_password(username, old_password, new_password):
        # TODO: arguments validity check
        lookup = next(user_db.execute('SELECT user_name,user_pass,user_pass_salt FROM user_info WHERE user_name=?',
                                      (username,)),
                      None)  # TODO: avoid blocking function call (not urgent, only take 30μs)
        if lookup is None:
            raise ValueError('user name not found')
        if old_password is not None:  # avoid check old_password if old_password is None
            salted_pass = old_password + lookup[2]
            stored_pass = hashlib.sha256(salted_pass.encode('utf-8')).hexdigest()
            if stored_pass != lookup[1]:
                raise ValueError('old_password is not correct')
        # change password now
        salt = (''.join(format(x, '02x') for x in os.urandom(64)))
        salted_pass = new_password + salt
        stored_pass = hashlib.sha256(salted_pass.encode('utf-8')).hexdigest()
        user_db.execute('UPDATE user_info SET user_pass=?, user_pass_salt=? WHERE user_name=?',
                        (stored_pass, salt, username))
        user_db.commit()
        return

    def get_user_level(username_b):
        if username_b is None:
            return -1
        else:
            username = username_b.decode('utf-8')
        lookup = next(user_db.execute('SELECT user_level FROM user_info WHERE user_name=?',
                                      (username,)), None)
        return lookup[0]

    class LoginHandler(tornado.web.RequestHandler):
        def post(self):
            try:
                username = self.get_argument("username")
                password = self.get_argument("password")  # TODO: arguments validity check
            except tornado.web.MissingArgumentError:
                self.set_status(400)
                self.write('missing arguments')
                self.finish()
                return
            lookup = next(user_db.execute('SELECT user_name,user_pass,user_pass_salt FROM user_info WHERE user_name=?',
                                          (username,)),
                          None)  # TODO: avoid blocking function call (not urgent, only take 30μs)
            if lookup is None:
                self.set_status(401)
                self.write('invalid user name and password combination')
                self.finish()
                return
            input_salted_pass = password + str(lookup[2])
            input_pass = hashlib.sha256(input_salted_pass.encode('utf-8')).hexdigest()
            if input_pass != str(lookup[1]):
                self.set_status(401)
                self.write('invalid user name and password combination')
                self.finish()
                return
            if self.request.protocol == 'http':
                self.set_secure_cookie("username", lookup[0].encode('utf-8'), secure=False, httponly=True)
            else:
                self.set_secure_cookie("username", lookup[0].encode('utf-8'), secure=True, httponly=True)
            self.set_status(200)
            self.redirect('/')
            return

    class LogoutHandler(tornado.web.RequestHandler):
        def get(self):
            self.clear_cookie("username")
            self.set_status(200)
            self.redirect('/')
            return

    class NewUserHandler(tornado.web.RequestHandler):
        def post(self):
            # first check if user is admin
            level = get_user_level(self.get_secure_cookie('username'))
            if level != 0:
                self.write('invalid user name, action not permitted')
                self.set_status(401)
                self.finish()
                return

            try:
                username = self.get_argument("username")
                password = self.get_argument("password")
                user_level = self.get_argument("user_level")
                create_user(username, password, int(user_level))
            except tornado.web.MissingArgumentError:
                self.write('missing arguments')
                self.set_status(400)
                self.finish()
                return
            except (TypeError, ValueError, sqlite3.Error) as ex:
                self.write('invalid new user request: ' + str(ex))
                self.set_status(400)
                self.finish()
                return
            self.redirect('/')

    class ChangePasswordHandler(tornado.web.RequestHandler):
        def post(self):
            level = get_user_level(self.get_secure_cookie('username'))
            try:
                if level == 0:
                    old_password = self.get_argument("old_password", default=None)
                else:
                    old_password = self.get_argument("old_password")
                change_password(self.get_argument("username"),
                                old_password,
                                self.get_argument("new_password"))
            except tornado.web.MissingArgumentError:
                self.write('missing arguments')
                self.set_status(400)
                self.finish()
                return
            except (ValueError, TypeError) as ex:
                self.write('invalid new user request: ' + str(ex))
                self.set_status(400)
                self.finish()
                return
            self.redirect('/')

    class WebHandlerMain(tornado.web.RequestHandler):
        def get(self):
            username_b = self.get_secure_cookie('username')
            if username_b is None:
                username = None
            else:
                username = username_b.decode('utf-8')
            level = get_user_level(username_b)
            mod_list = res.mod_manager.web_content_all_list(self, username)
            self.render("control_panel.html", mod_list=mod_list, username=username, user_level=level)

    user_db = sqlite3.connect('users.db')
    user_db.execute("CREATE TABLE if not exists user_info ("
                    "user_name varchar(64) PRIMARY KEY NOT NULL,"
                    "user_pass varchar(64) NOT NULL,"
                    "user_pass_salt varchar(32) NOT NULL,"
                    "user_level SMALLINT NOT NULL"
                    ");")
    user_db.commit()
    if len_i(user_db.execute('SELECT user_name FROM user_info')) == 0:
        create_user('admin', 'admin', 0)

    handler_list = [(r"/", WebHandlerMain),
                    (r"/login", LoginHandler),
                    (r"/logout", LogoutHandler),
                    (r"/new_user", NewUserHandler),
                    (r"/change_password", ChangePasswordHandler),
                    (r"/.well-known/acme-challenge/(.*)", tornado.web.StaticFileHandler, {
                        "path": os.path.join(os.path.dirname(__file__), "lets_encrypt_webroot", ".well-known",
                                             "acme-challenge")}),
                    (r"/static/(.*)", tornado.web.StaticFileHandler,
                     {"path": os.path.join(os.path.dirname(__file__), "templates", "static")})] + \
                   res.mod_manager.web_post_handler_list_get()
    app = tornado.web.Application(
        handler_list,
        template_path=os.path.join(os.path.dirname(__file__), "templates"),
        autoescape=None,
        cookie_secret="FJionNONoi89Hoi9h(*Y*(hU*(BHIUy78HoBHUIBFHiFHG78e",
        ui_modules=web_server_ui_module)
    if http_server_port > 0:
        http_server = tornado.httpserver.HTTPServer(app)
        http_server.listen(http_server_port)
    if https_server_port > 0:
        ssl_cert = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        ssl_cert.load_cert_chain(os.path.join('/', 'etc', 'letsencrypt', 'live', SERVER_DOMAIN, 'fullchain.pem'),
                                 os.path.join('/', 'etc', 'letsencrypt', 'live', SERVER_DOMAIN, 'privkey.pem'))
        https_server = tornado.httpserver.HTTPServer(app, ssl_options=ssl_cert)
        https_server.listen(https_server_port)
