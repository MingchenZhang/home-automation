import base64
import json

from .log import *
from tornado import ioloop
import socket as so


class Level1Connector:
    RF24_BASE_ADDRESS = 0xaaaaaaaa00
    RF24_LOCAL_ADDRESS = RF24_BASE_ADDRESS + 1

    def __init__(self, res, listen_port):
        self.res = res
        self.server_socket = so.socket(so.AF_INET, so.SOCK_STREAM)
        self.server_socket.setsockopt(so.SOL_SOCKET, so.SO_REUSEADDR, 1)
        self.server_socket.bind(('', listen_port))
        self.server_socket.listen(20)
        self.server_socket.settimeout(0.0)
        ioloop.IOLoop.current().add_handler(self.server_socket, self.new_connection, ioloop.IOLoop.READ)
        # res.selector.register(self.server_socket, EVENT_READ, self.new_connection)
        info('Level1Connector now listen on ' + str(listen_port))
        self.clients_conn_client = []  # store (level1DevSocket,itsClient) # TODO: could be optimized
        self.clients_conn_buffer = {}  # store socket : bufferSpace
        self.clients_conn_addr = {}  # store socket : addrInfo

    def new_connection(self, socket, event):
        assert socket is self.server_socket
        client_socket, addr = self.server_socket.accept()
        client_socket.setsockopt(so.IPPROTO_TCP, so.TCP_NODELAY, 1)
        info('New connection to Level1Connector from ' + str(addr))
        client_socket.settimeout(0.0)
        self.clients_conn_buffer[client_socket] = bytearray()
        self.clients_conn_addr[client_socket] = addr
        ioloop.IOLoop.current().add_handler(client_socket, self.new_message, ioloop.IOLoop.READ)
        # self.res.selector.register(client_socket, EVENT_READ, self.new_message)

    def new_message(self, socket, event):
        assert socket in self.clients_conn_buffer
        try:
            new_data = socket.recv(2*1024*1024)
        except ConnectionError:
            new_data = None
        if not new_data:
            info('Level1 device ' + str(self.clients_conn_addr[socket]) + ' disconnected')
            ioloop.IOLoop.current().remove_handler(socket)
            # self.res.selector.unregister(socket)
            socket.close()
            # TODO_: delete self.clients_conn_client entry
            self.clients_conn_client = [(sock, address) for sock, address in self.clients_conn_client if socket != sock]
            del self.clients_conn_buffer[socket]
            del self.clients_conn_addr[socket]
            return
        self.clients_conn_buffer[socket] += new_data
        while True:
            newline_index = self.clients_conn_buffer[socket].find(bytes([0x0A]))  # find new line
            if newline_index != -1:
                newline = self.clients_conn_buffer[socket][:newline_index]
                self.clients_conn_buffer[socket] = self.clients_conn_buffer[socket][newline_index + 1:]
                try:
                    newline_str = newline.decode('utf-8')
                except ValueError:
                    warn('New message from ' + str(self.clients_conn_addr[socket]) + ' cannot be processed')
                    continue
                debug('New message received from ' + str(self.clients_conn_addr[socket]) + ':')
                debug(newline_str)
                self.process_message(socket, newline_str)
            else:
                break

    def process_message(self, sock, newline):
        import json
        try:
            message = json.loads(newline)  # TODO_: might raise ValueError and TypeError
        except ValueError and TypeError:
            warn('New message from ' + str(self.clients_conn_addr[sock]) + ' cannot be decoded')
            return

        def rf24_packet():
            if 'packet' not in message and type(message['packet']) is str:
                invalid_message()
                return
            try:
                import binascii
                packet_content = binascii.unhexlify(message['packet'])  # TODO_: TypeError
            except TypeError and ValueError:
                error('Message from level1: `' + str(message) + '\' packet field is invalid (rf24_packet)')
                return
            if len(packet_content) < 11:
                error('Message from level1: `' + str(message) + '\' contain a undersized rf24 packet')
                return
            if len(packet_content) > 32:
                error('Message from level1: `' + str(message) + '\' contain a oversize rf24 packet')
                return
            dest = translateAddressToNum(packet_content[6:11])
            if dest == Level1Connector.RF24_LOCAL_ADDRESS:
                debug('Message from level1: `' + str(message) + '\' rf24 packet received locally')
                scr = translateAddressToNum(packet_content[1:6])
                self.res.mod_manager.new_rf24_message(int(packet_content[0]), scr, packet_content[11:])
            else:
                debug('Message from level1: `' + str(message) + '\' rf24 packet forwarded')
                self.send_to(int(packet_content[0]), dest, packet_content[11:])
            return

        def device_list():
            if 'device_list' not in message and type(message['device_list']) is list:
                invalid_message()
                return
            available_addr_list = []
            for device in (x for x in message['device_list'] if type(x) is dict):
                if 'address' in device and type(device['address']) is int:
                    address = device['address']
                    if 0 <= address <= 0xFFFFFFFFFF:
                        available_addr_list.append(address)
            debug('device_list received from ' + str(self.clients_conn_addr[sock]) + ' (' + str(
                available_addr_list) + ')')
            self.set_refresh_clients(sock, available_addr_list)
            return

        def event():
            try:
                self.res.mod_manager.trigger_event(message['event_key'], event_data=message['data'])
            except LookupError:
                warn('received invalid event from ' + str(self.clients_conn_addr[sock]))
            return

        def ping():
            sock.sendall((json.dumps({'type': 'ping_respond'}) + '\n').encode('utf-8'))
            return

        def invalid_message():
            error('Message from level1: `' + str(message) + '\' is invalid')
            return

        if 'type' in message:
            {'rf24_packet': rf24_packet,
             'device_list': device_list,
             'ping': ping,
             'event': event,
             }.get(message['type'], invalid_message)()
        return

    def request_refresh_clients(self):
        import json
        for sock in self.clients_conn_buffer:
            request = {'type': 'device_list_get'}
            sock.sendall((json.dumps(request) + '\n').encode('utf-8'))

    def set_refresh_clients(self, sock, dev_list):
        # TODO: could be optimized
        self.clients_conn_client = [(socket, address) for socket, address in self.clients_conn_client if socket != sock]
        for address in dev_list:
            self.clients_conn_client.append((sock, address))

    def send_to(self, type, dest, content_bytes):
        if len(content_bytes) > 22:
            error('send_to receive a oversize content_bytes, message not send')
            return False
        import binascii
        packet_content = bytes(
            bytes([type]) +
            translateAddressToBig(Level1Connector.RF24_LOCAL_ADDRESS) +
            translateAddressToBig(dest) +
            content_bytes)
        packet = {'type': 'rf24_packet', 'packet': (''.join(format(x, '02x') for x in packet_content)).upper()}
        # find where to send (forwarding lookup)
        socket_to_use = None
        for (sock, address) in self.clients_conn_client:
            if address == dest:
                socket_to_use = sock
        # encode and send
        if socket_to_use is None:
            error('No forwarding entry found for ' + hex(dest))
            return False
        packet_str = json.dumps(packet)
        socket_to_use.sendall((packet_str + '\n').encode('utf-8'))
        debug('New message send to ' + str(self.clients_conn_addr[sock]) + ':')
        debug(packet_str)
        return True

    def send_json_to(self, dest, json_dict):
        socket_to_use = None
        for (sock, address) in self.clients_conn_client:
            if address == dest:
                socket_to_use = sock
        if socket_to_use is None:
            error('No forwarding entry found for ' + hex(dest))
            return False
        try:
            packet_str = json.dumps(json_dict)
        except (ValueError, TypeError):
            error('dictionary dump to bytes failed')
            return False
        try:
            socket_to_use.sendall((packet_str + '\n').encode('utf-8'))
        except Exception:
            error('send to ' + str(self.clients_conn_addr[sock]) + ' failed')
            return False
        return True

    def is_exist_in_network(self, addr):
        for (sock, address) in self.clients_conn_client:
            if address == addr:
                return True
        return False


def translateAddressToBig(address_num):
    """ this method return a bytes object containing a representation of address_num in big endian """
    addr_b = bytearray()
    addr_b[0:5] = [0] * 5
    for i in reversed(range(1, 6)):
        addr_b[i - 1] = (address_num >> (8 * (5 - i))) & 0xFF
    return bytes(addr_b)


def translateAddressToNum(address_b):
    """ this method return the address number from a big endian byte array """
    addr_n = 0
    for i in reversed(range(1, 6)):
        addr_n += address_b[i - 1] << (8 * (5 - i))
    return addr_n
