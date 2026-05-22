# Copyright (c) Arduino s.r.l. and/or its affiliated companies
# SPDX-License-Identifier: Apache-2.0

import time
from arduino.app_utils import *

led_state = False

def loopback(message):
    time.sleep(1)
    return message

Bridge.provide("loopback", loopback)
App.run()
