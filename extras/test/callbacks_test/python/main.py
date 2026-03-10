# This file is part of the Arduino_RouterBridge library.
#
# Copyright (C) Arduino s.r.l. and/or its affiliated companies
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
#     file, You can obtain one at http://mozilla.org/MPL/2.0/.

import time
from arduino.app_utils import *


def no_args_no_result():
    print("0 args no result")
    return

def no_args_bool_result():
    print("0 args bool result")
    return True

def one_args_float_result(a):
    print("1 args float result")
    return a^2

def two_args_float_result(a, b):
    print("2 args float result")
    return a/b

def one_args_no_result(msg: str):
    print("1 args no result")
    print(f"Message received: {msg}")

def two_args_no_result(msg: str, x: int):
    print("2 args no result")
    print(f"Message received: {msg} {str(x)}")


if __name__ == "__main__":

    Bridge.provide("0_args_no_result", no_args_no_result)
    Bridge.provide("0_args_bool_result", no_args_bool_result)
    Bridge.provide("1_args_float_result", one_args_float_result)
    Bridge.provide("2_args_float_result", two_args_float_result)
    Bridge.provide("1_args_no_result", one_args_no_result)
    Bridge.provide("2_args_no_result", two_args_no_result)

    App.run()
