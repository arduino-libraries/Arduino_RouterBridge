/*
  This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Udp NTP Client

    Get the time from a Network Time Protocol (NTP) time server
    Demonstrates use of UDP sendPacket and ReceivePacket
    For more on NTP time servers and the messages needed to communicate with them,
    see https://en.wikipedia.org/wiki/Network_Time_Protocol

    Example freely inspired from the Arduino Ethernet library
    https://github.com/arduino-libraries/Ethernet/blob/master/examples/UdpNtpClient/UdpNtpClient.ino
  
*/

#include <Arduino_RouterBridge.h>

unsigned int localPort = 8888;
const char timeServer[] = "time.nist.gov";
int NTP_PACKET_SIZE = 48;

BridgeUDP<4096> Udp(Bridge);

void setup() {
  //Bridge.begin();   // optional when Serial.begin is called
  Serial.begin();     // same as Monitor.begin();
  Udp.begin(localPort);
  Udp.setTimeout(1000);
}

void loop() {
  sendNTPpacket(timeServer);

  if (Udp.parsePacket()) {
    byte packetBuffer[NTP_PACKET_SIZE];
    Udp.read(packetBuffer, NTP_PACKET_SIZE);

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long.
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    Serial.print("Unix time = ");
    Serial.println(epoch);

    Serial.print("The UTC time is ");
    Serial.print((epoch  % 86400L) / 3600);
    Serial.print(':');
    if (((epoch % 3600) / 60) < 10) {
      Serial.print('0');
    }
    Serial.print((epoch % 3600) / 60);
    Serial.print(':');
    if ((epoch % 60) < 10) {
      Serial.print('0');
    }
    Serial.println(epoch % 60);
  }

  delay(10000);
}

void sendNTPpacket(const char * address) {
  byte packetBuffer[NTP_PACKET_SIZE];
  
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  if (Udp.beginPacket(address, 123)) {  // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
  } else {
    Serial.println("Failed to send NTP request.");
  }
}

