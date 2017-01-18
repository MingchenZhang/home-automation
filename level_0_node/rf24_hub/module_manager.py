import importlib
import traceback

from .modules.base_module import BaseModule
from .log import *
from threading import RLock
import uuid


class ModuleManager:
    def __init__(self, res):
        self.res = res
        self.res.mod_manager = self
        self.mod_list = []  # list of module object
        self.uuid_mod = {}  # uuid and corresponding module
        self.addr_mod = {}  # store addr : [modules]
        self.event_mod = {}  # store eventString : [(module, callback, subscriber_data)]
        self.mod_lock = {}  # module and it Rlock
        modules_config = importlib.import_module('rf24_hub.modules')
        for mod in modules_config.ALL_MODULES:
            mod_m = importlib.import_module('rf24_hub.modules.' + mod)
            mod_c = getattr(mod_m, mod)
            mod_uuid = uuid.uuid4()
            mod_o = mod_c(res, mod_uuid)  # TODO_: add all required parameter to work
            if isinstance(mod_o, BaseModule):
                # get all address subscription
                address_list = mod_o.get_rf24_listen_addr()
                address_list = [addr for addr in address_list if addr >= 0xaaaaaaaa00] + \
                               [addr + 0xaaaaaaaa00 for addr in address_list if addr < 0xaaaaaaaa00]
                for address in address_list:
                    if address in self.addr_mod:
                        self.addr_mod[address].append(mod_o)
                    else:
                        self.addr_mod[address] = [mod_o]
                # create thread lock
                self.mod_lock[mod_o] = RLock()
                # add module into the list
                self.mod_list.append(mod_o)
                self.uuid_mod[mod_uuid] = mod_o
                debug('new module generated (name:' + mod + ') (uuid:' + str(mod_uuid) + ')')
            else:
                warn('module ' + mod + ' in ' + 'rf24_hub.modules.' + mod + ' is illegal')

    def module_lock(self, mod_to_lock, block=True):
        # assert mod_to_lock in self.mod_lock
        # return self.mod_lock[mod_to_lock].acquire(block)
        return True  # now is single threaded

    def module_unlock(self, mod_to_lock):
        # assert mod_to_lock in self.mod_lock
        # self.mod_lock[mod_to_lock].release()
        return True  # now is single threaded

    def new_rf24_message(self, packet_type, scr_addr, packet_content):
        # TODO_: call corresponding module
        if scr_addr in self.addr_mod:
            assert type(self.addr_mod[scr_addr]) is list
            for module in self.addr_mod[scr_addr]:
                try:
                    module.handle_new_rf24_message(packet_type, scr_addr, packet_content)
                except Exception:
                    debug(traceback.format_exc())
        return

    def subscribe_event(self, subscriber, event_key, callback, subscriber_data=None):
        # if subscriber not in self.mod_list:
        #     raise ValueError('Subscriber is not a module')  # TODO: make a new initialization method for all modules, instead of using __init__()
        if not callable(callback):
            raise TypeError('Callback should be a function')
        if event_key in self.event_mod:
            assert type(self.event_mod[event_key]) is list
            self.event_mod[event_key].append((subscriber, callback, subscriber_data))
        else:
            self.event_mod[event_key] = [(subscriber, callback, subscriber_data)]

    def subscribe_event_remove(self, subscriber, event_key):
        if event_key in self.event_mod:
            self.event_mod[event_key] = [(s, c, d) for s, c, d in self.event_mod[event_key] if s == subscriber]

    def trigger_event(self, event_key, event_data=None):
        if type(event_key) is not str:
            raise TypeError('Event key should be a string')
        if event_key in self.event_mod:
            for subscriber, callback, subscriber_data in self.event_mod[event_key]:
                if subscriber not in self.mod_list:
                    warn('Event subscriber is not in mod_list')
                self.module_lock(subscriber)
                callback(subscriber_data, event_data)
                self.module_unlock(subscriber)

    def web_content_all_list(self, conn_handler, username):
        content_list = []
        for module in self.mod_list:
            if self.module_lock(module, block=False):
                module_str = module.web_gui_generate(conn_handler, username)
                self.module_unlock(module)
            else:
                module_str = 'module busy'
            if not isinstance(module_str, str) and \
                    not isinstance(module_str, bytes) and \
                    not isinstance(module_str, list):
                module_str = 'module return abnormal'  # TODO: better warning
            if isinstance(module_str, str):
                content_list.append(module_str)
            elif isinstance(module_str, bytes):
                content_list.append(module_str.decode('utf-8'))
            else:
                for mod in module_str:
                    if isinstance(mod, str):
                        content_list.append(mod)
                    elif isinstance(mod, bytes):
                        content_list.append(mod.decode('utf-8'))
                    else:
                        raise TypeError('web_gui_generate should return a str or bytes, or a list of them')
        return content_list

    def web_content_get(self, _uuid, conn_handler):
        if _uuid in self.uuid_mod:
            mod = self.uuid_mod[_uuid]
            assert mod.uuid == _uuid
            if self.module_lock(mod, block=False):
                module_str = mod.web_gui_generate(conn_handler)
                self.module_unlock(mod)
            else:
                module_str = 'module busy'  # TODO: better warning
            if not isinstance(module_str, str) and \
                    not isinstance(module_str, bytes) and \
                    not isinstance(module_str, list):
                module_str = 'module return abnormal'  # TODO: better warning
            if isinstance(module_str, str):
                return [module_str]
            elif isinstance(module_str, bytes):
                return [module_str.decode('utf-8')]
            else:
                mod_list = []
                for mod in module_str:
                    mod_list.append(mod)
                return mod_list
        else:
            return None

    # def web_post(self, _uuid, post_str):
    #     if _uuid in self.uuid_mod:
    #         mod = self.uuid_mod[_uuid]
    #         assert mod.uuid == _uuid
    #         if self.module_lock(mod, block=False):
    #             result_str = mod.web_gui_post(post_str)
    #             self.module_unlock(mod)
    #         else:
    #             result_str = 'module busy'  # TODO: better warning
    #         return result_str
    #     else:
    #         return None

    def web_post_handler_list_get(self):
        l = []
        for mod in self.mod_list:
            l.append((r"/module/" + str(mod.uuid) + '(.*)', mod.get_ajax_handler()))
        return l
