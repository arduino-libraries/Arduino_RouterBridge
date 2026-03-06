# A python sketch that uses RPC Bridge to test the server.ino

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
