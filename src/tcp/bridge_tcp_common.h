/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#pragma once
#ifndef BRIDGE_TCP_COMMON_H
#define BRIDGE_TCP_COMMON_H


class BridgeTCPConnection {
public:
    virtual ~BridgeTCPConnection() = default;
    virtual void disconnect() = 0;
};

#endif //BRIDGE_TCP_COMMON_H