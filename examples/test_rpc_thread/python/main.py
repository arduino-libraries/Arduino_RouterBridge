# SPDX-FileCopyrightText: Copyright (C) 2025 ARDUINO SA <http://www.arduino.cc>
#
# SPDX-License-Identifier: MPL-2.0

import time
from arduino.app_utils import *

led_state = False

def loopback(message):
    time.sleep(1)
    return message

Bridge.provide("loopback", loopback)
App.run()
