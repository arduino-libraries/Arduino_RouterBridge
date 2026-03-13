# A python sketch that uses RPC Bridge to test the server.ino

import time
from arduino.app_utils import *

led_state = False


def log(msg):
    with open("./log.log", "a") as f:
        f.write(str(msg) + "\n")


def signal(val):
    log(f"Received signal: {val}")


def loop():
    global led_state

    if Bridge.call("set_led", led_state) == led_state:
        led_state = not led_state
        log(f"LED toggled to {led_state}")
    else:
        log("LED toggle failed")

    time.sleep(1)

    if Bridge.call("add", 10, 15) == 25:
        log("add function working")
    else:
        log("add function not working")

    time.sleep(1)

    log(f"greeting message: {Bridge.call('greet')}")

    time.sleep(1)

Bridge.provide("signal", signal)

App.run(user_loop=loop)
