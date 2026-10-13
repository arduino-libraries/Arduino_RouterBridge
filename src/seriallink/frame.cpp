/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include "frame.h"

#include <string.h>

#include "crc32.h"

namespace seriallink {

void putUint32(uint8_t* b, uint32_t v) {
    b[0] = v;
    b[1] = v >> 8;
    b[2] = v >> 16;
    b[3] = v >> 24;
}

uint32_t getUint32(const uint8_t* b) {
    return uint32_t(b[0]) | uint32_t(b[1]) << 8 | uint32_t(b[2]) << 16 | uint32_t(b[3]) << 24;
}

size_t encodeFrame(const Frame& f, uint8_t* raw, uint8_t* dst) {
    raw[0] = PROTOCOL_VERSION << 4 | uint8_t(f.type);
    raw[1] = f.seq;
    raw[2] = f.ack;
    raw[3] = f.wnd;
    if (f.len > 0) memcpy(raw + HEADER_SIZE, f.payload, f.len);
    const size_t bodyLen = HEADER_SIZE + f.len;
    putUint32(raw + bodyLen, crc32(raw, bodyLen));
    const size_t n = cobs::encode(dst, raw, bodyLen + CRC_SIZE);
    dst[n] = 0;
    return n + 1;
}

bool decodeFrame(const uint8_t* encoded, size_t len, uint8_t* raw, size_t rawCap, Frame& f) {
    size_t rawLen;
    if (!cobs::decode(raw, rawCap, encoded, len, rawLen)) return false;
    if (rawLen < HEADER_SIZE + CRC_SIZE) return false;
    const size_t bodyLen = rawLen - CRC_SIZE;
    if (crc32(raw, bodyLen) != getUint32(raw + bodyLen)) return false;
    if (raw[0] >> 4 != PROTOCOL_VERSION) return false;
    const uint8_t type = raw[0] & 0x0F;
    if (type < uint8_t(FrameType::Data) || type > uint8_t(FrameType::Rst)) return false;
    f.type = FrameType(type);
    f.seq = raw[1];
    f.ack = raw[2];
    f.wnd = raw[3];
    f.payload = raw + HEADER_SIZE;
    f.len = bodyLen - HEADER_SIZE;
    return true;
}

size_t marshalSyn(const SynParams& p, bool withEcho, uint8_t* out) {
    out[0] = p.maxPayload;
    out[1] = p.maxPayload >> 8;
    out[2] = p.window;
    putUint32(out + 3, p.nonce);
    if (!withEcho) return SYN_SIZE;
    putUint32(out + 7, p.echo);
    return SYN_ACK_SIZE;
}

bool parseSyn(const uint8_t* b, size_t len, bool withEcho, SynParams& p) {
    if (len != (withEcho ? SYN_ACK_SIZE : SYN_SIZE)) return false;
    p.maxPayload = uint16_t(b[0] | b[1] << 8);
    p.window = b[2];
    p.nonce = getUint32(b + 3);
    p.echo = withEcho ? getUint32(b + 7) : 0;
    if (p.maxPayload < 1 || p.maxPayload > MAX_PAYLOAD_LIMIT) return false;
    if (p.window < 1 || p.window > MAX_WINDOW_LIMIT) return false;
    return true;
}

} // namespace seriallink
