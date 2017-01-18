import abc

from .. import web_server
from ..level1_connector import Level1Connector
import tornado.web


class BaseModule:
    def __init__(self, res, uuid):
        self.res = res
        self.uuid = uuid
        pass

    def is_exist_in_network(self, node_num):
        return self.res.level_1_conn.is_exist_in_network(Level1Connector.RF24_BASE_ADDRESS + node_num)

    def send_rf24_packet(self, _type, node_num, content_bytes):
        # TODO: error checking
        if type(_type) is int and type(node_num) is int and type(content_bytes) is bytes:
            return self.res.level_1_conn.send_to(_type, Level1Connector.RF24_BASE_ADDRESS + node_num, content_bytes)
        else:
            raise TypeError('send_rf24_packet received illegal type')

    def send_json_to_device(self, node_no, json_dict):
        self.res.level_1_conn.send_json_to(Level1Connector.RF24_BASE_ADDRESS + node_no, json_dict)

    def schedule_to_run(self, time, callback, key=None):
        """
        schedule a time to call the callback
        :param time: when to call in terms of unix timestamp
        :param callback: Callback used.
        It must has one positional param if key is not None to receive key
        :param key: data to pass to callback
        :return: No return
        """
        self.res.scheduler.add_schedule(self, time, callback, key)

    def subscribe_event(self, event_key, callback, subscriber_data=None):
        """
        subscribe an event
        :param event_key: A string of the event key
        :param callback: A callback once the event is triggered.
        It must has following positional param: subscriber_data, event_data
        :param subscriber_data: Data field that will give back in event callback
        :return: No return
        """
        self.res.mod_manager.subscribe_event(self, event_key, callback, subscriber_data)

    @abc.abstractmethod
    def get_rf24_listen_addr(self):
        """
        Modules must implement this method to subscribe to a set of node number.
        Return an empty list if no rf24 packet is needed for this module
        :return: a list of address numbers
        """
        return

    @abc.abstractmethod
    def handle_new_rf24_message(self, packet_type, scr_addr, packet_content):
        """
        if a message from subscribed address arrive, this method will be called
        :param packet_type: Integer packet type.
        :param scr_addr: node number that incoming message came from
        :param packet_content: packet content [11:]
        :return: No return
        """
        return

    @abc.abstractmethod
    def web_gui_generate(self, conn_handler, username):
        """
        Return the html content that will be displayed in the web management page
        :param conn_handler: tornado.web.RequestHandler instance used to handle the page, should the module needs
        :param username: username Extract from RequestHandler. Could be used to generate user specific web page.
        It will be None if the user has not logged in.
        :return: could return str, bytes or a list of str or bytes
        """
        return

    def get_ajax_handler(self):
        """
        Override this method if the module needs to handle http request from web page.
        All request should be send to /module/(uuid)
        uuid could be retried from self.uuid.
        More detail is in tornado documentation: http://www.tornadoweb.org/en/stable/web.html
        :return: a tornado.web.RequestHandler to handle
        """
        class PostHandler(tornado.web.RequestHandler):
            def get(self): pass
            def post(self):  pass
        return PostHandler