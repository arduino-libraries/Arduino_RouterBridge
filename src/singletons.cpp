#define UNO_Q_MONITOR_ALIAS
#include "Arduino_RouterBridge.h"

#ifndef UNO_Q_SERIAL_RPC
#define UNO_Q_SERIAL_RPC Serial1
#endif

BridgeClass Bridge(UNO_Q_SERIAL_RPC);
BridgeMonitor<> Monitor(Bridge);
