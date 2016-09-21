/*
 * NDNOverUDP, Library for a Arduino NDN Router/Producer.
 * Copyright (C) 2016  Antonio Cardace, Davide Aguiari.
 *
 * NDNOverUDP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Wavetrack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef NDNOverUDP_h
#define NDNOverUDP_h

#include "Arduino.h"
#include <EthernetUdp.h>

#ifndef __ARDUINO_X86__
#include <SPI.h>
#endif

#define NDN_PORT 8888

/* NDN Packet types */
#define NDN_INTEREST_PACKET 0x1
#define NDN_DATA_PACKET 0x2

#define NDN_HEADER_SIZE_INTEREST 0x7
#define NDN_HEADER_SIZE_DATA 0x7

#define UDP_BUFFER_SIZE 256 // 1KB

// if the arduino reaches these limits it will drom any future incoming packet
// of course until it has fullfilled some requests
#define NDN_ROUTING_TABLE_SIZE 10 // up to 10 unique outstanding interests
#define NDN_ROUTING_HASH_SIZE 16
// 5 seconds of ttl, after that the pending interest gets dropped
#define NDN_ROUTING_TTL 5000
typedef struct NDNRouteEntry {
  boolean freeBlock;
  unsigned long nonce;
  uint8_t interestHash[NDN_ROUTING_HASH_SIZE]; // hash from SpritzCipher.h
  IPAddress ip;
  unsigned long timestamp;
} NDNRouteEntry;

typedef NDNRouteEntry NDNRoutingTable[NDN_ROUTING_TABLE_SIZE];

typedef struct __attribute__((packed)) NDNInterestPacket {
#ifdef __ARDUINO_X86__
  /* if you are an Intel Galileo you're dumb so trust me you need this */
  unsigned long ip;
#endif
  byte type;
  unsigned long nonce;
  unsigned short nameLength;
  char *name;
} NDNInterestPacket;

typedef struct __attribute__((packed)) NDNDataPacket {
#ifdef __ARDUINO_X86__
  unsigned long ip;
#endif
  byte type;
  unsigned short nameLength;
  unsigned long contentLength;
  char *name;
  char *content;
} NDNDataPacket;

/* function type associated with an interest name */
typedef int (*dataProducer)(char **content);

class NDNOverUDP {
public:
  NDNOverUDP();
  int begin(byte macAddress[6]);
  void begin(byte macAddress[6], IPAddress ipAddress);
  void stop();
  int publishInterests(char **names, dataProducer functions[], unsigned int n);
  void startDaemon();
  void dumpRoutingTable();
#ifdef __ARDUINO_X86__
  int addNDNNodes(IPAddress ipAddress[], unsigned int n);
#endif

private:
  void sendInterest(NDNInterestPacket *packet);
  void sendData(IPAddress ipDest, char *name, unsigned short nameLength,
                char *content, unsigned long contentLength);
  void sendData(IPAddress ipDest, NDNDataPacket *dataPkt);
  void receiveInterest(char *packetBuffer, NDNInterestPacket *interestPkt);
  void receiveData(char *packetBuffer, NDNDataPacket *dataPkt);
#ifdef __ARDUINO_X86__
  void multicastSendInterest(NDNInterestPacket *pkt);
#else
  void broadcastSendInterest(NDNInterestPacket *pkt);
#endif

  static void freeInterestPacket(NDNInterestPacket *pkt);
  static void freeDataPacket(NDNDataPacket *pkt);

  /* NDN Routing Table functions */
  bool setRoute(NDNInterestPacket *pkt);
  bool inRoutingTable(char *name, uint16_t nameLength, unsigned long nonce);
  NDNRouteEntry *getRoute(char *name, uint16_t nameLength);
  void deleteRoute(char *name, uint16_t nameLength);
  void dropExpiredInterest();

  /* DEBUG routines */
  static void dumpInterestPacket(NDNInterestPacket *pkt);
  static void hexDumpInterestPacket(NDNInterestPacket *pkt);
  static void dumpDataPacket(NDNDataPacket *pkt);
  static void hexDumpDataPacket(NDNDataPacket *pkt);
  static void hexDump(char buffer[], int n);

  NDNRoutingTable _routingTable;
  byte _routingTableSize;
  byte _routingFreeEntryIndex;
  EthernetUDP _udpIstance;
  IPAddress *_nodes;
  char *_packetBuffer;
  char **_interests;
  dataProducer *_interestsFunctions;
  unsigned int _numOfNodes;
  unsigned int _numOfInterests;
#ifdef __ARDUINO_X86__
  IPAddress _senderAddr;
#endif
};

#endif
