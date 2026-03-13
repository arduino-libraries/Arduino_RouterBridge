/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#define UNO_Q_MONITOR_ALIAS
#include "Arduino_RouterBridge.h"

#ifndef UNO_Q_SERIAL_RPC
#define UNO_Q_SERIAL_RPC Serial1
#endif

BridgeClass Bridge(UNO_Q_SERIAL_RPC);
BridgeMonitor<> Monitor(Bridge);
