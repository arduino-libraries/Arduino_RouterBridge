# This file is part of the Arduino_RouterBridge library.
#
# Copyright (C) Arduino s.r.l. and/or its affiliated companies
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
#     file, You can obtain one at http://mozilla.org/MPL/2.0/.

import time
from arduino.app_utils import *

led_state = False

def loopback(message):
    time.sleep(1)
    return message

Bridge.provide("loopback", loopback)
App.run()
