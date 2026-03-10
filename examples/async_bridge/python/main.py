# This file is part of the Arduino_RouterBridge library.
#
# Copyright (C) Arduino s.r.l. and/or its affiliated companies
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
#     file, You can obtain one at http://mozilla.org/MPL/2.0/.

# A python sketch that uses RPC Bridge to test the async_bridge.ino

import time
from arduino.app_utils import *


def log(msg):
    with open("./log.log", "a") as f:
        f.write(str(msg) + "\n")


def multiply(a, b):
    return a * b


def loop():
    message = "Hello from Python!"

    Bridge.notify("mailme", message)

    time.sleep(1)

Bridge.provide("multiply", multiply)

App.run(user_loop=loop)
