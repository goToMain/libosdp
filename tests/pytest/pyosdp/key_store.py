#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

import os
import tempfile
import random

class KeyStore():
    def __init__(self, dir=None):
        self.temp_dir = None
        if not dir:
            self.temp_dir = tempfile.TemporaryDirectory()
            self.key_dir = self.temp_dir.name
        else:
            self.key_dir = dir

    def key_file(self, name):
        return os.path.join(self.key_dir, 'key_' + name + '.bin')

    @staticmethod
    def gen_key(key_len=16):
        key = []
        for i in range(key_len):
            key.append(random.randint(0, 255))
        return bytes(key)

    def store_key(self, name, key, key_len=16):
        if not key or not isinstance(key, bytes) or len(key) != key_len:
            raise RuntimeError
        with open(self.key_file(name), "w") as f:
            f.write(key.hex())

    def load_key(self, name, key_len=16, create=False):
        if not os.path.exists(self.key_file(name)):
            if create:
                key = self.gen_key()
                self.store_key(name, key)
                return key
            raise RuntimeError
        with open(self.key_file(name), "r") as f:
            key = bytes.fromhex(f.read())
        if not key or not isinstance(key, bytes) or len(key) != key_len:
            raise RuntimeError
        return key

    def __del__(self):
        if self.temp_dir:
            self.temp_dir.cleanup()