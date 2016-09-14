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

#include "NDNOverUDP.h"
#include "Arduino.h"
#include <Ethernet.h>
#include <SpritzCipher.h>

#ifndef __ARDUINO_X86__
#include <utility/util.h>
#endif

NDNOverUDP::NDNOverUDP() {}

int NDNOverUDP::begin(byte macAddress[6]) {
  int obtainedFromDHCP = Ethernet.begin(macAddress);
  if (obtainedFromDHCP) {
    _udpIstance.begin(NDN_PORT);
    _packetBuffer = new char[UDP_BUFFER_SIZE];
    _nodes = NULL;
    _interests = NULL;
    _interestsFunctions = NULL;
    _numOfInterests = _numOfNodes = 0;
    _routingTableSize = 0;
    _routingFreeEntryIndex = 0;
    // trick: init all to 1 to make them all free blocks
    memset((void *)_routingTable, 1, sizeof(NDNRoutingTable));
  }
  return obtainedFromDHCP;
}

void NDNOverUDP::begin(byte macAddress[6], IPAddress ipAddress) {
  Ethernet.begin(macAddress, ipAddress);
  _udpIstance.begin(NDN_PORT);
  _packetBuffer = new char[UDP_BUFFER_SIZE];
  _nodes = NULL;
  _interests = NULL;
  _interestsFunctions = NULL;
  _numOfInterests = _numOfNodes = 0;
  _routingTableSize = 0;
  _routingFreeEntryIndex = 0;
}

void NDNOverUDP::stop() {
  _udpIstance.stop();
  if (_nodes != NULL) {
    delete[] _nodes;
  }
  if (_interests != NULL) {
    for (int i = 0; i < _numOfInterests; i++) {
      delete[] _interests[i];
    }
    delete[] _interests;
    delete[] _interestsFunctions;
  }
  delete[] _packetBuffer;
}

/* add the ndn Nodes to be managed by the NDN forwarder
   returns:
    0 - unsuccessful
    1 - successful
*/
#ifdef __ARDUINO_X86__
int NDNOverUDP::addNDNNodes(IPAddress ipAddress[], unsigned int n) {
  /* this function can only be called once at startup (after begin)*/
  if (!_numOfNodes) {
    _numOfNodes = n;
    _nodes = new IPAddress[n];
    memcpy((void *)_nodes, (void *)ipAddress, sizeof(IPAddress) * n);
    return 1;
  }
  return 0;
}
#endif

/* Publishes the interests owned by a producer and how to produce their
   contents.
   returns:
    0 - unsuccessful
    1 - successful
*/
int NDNOverUDP::publishInterests(char **names, dataProducer functions[],
                                 unsigned int n) {
  /* this function can only be called once at startup (after begin)*/
  unsigned int len;
  if (!_numOfInterests) {
    _numOfInterests = n;
    _interests = new char *[n];
    _interestsFunctions = new dataProducer[n];
    for (int i = 0; i < n; i++) {
      len = strlen(names[i]) + 1;
      _interests[i] = new char[len];
      strncpy(_interests[i], names[i], len);
      _interestsFunctions[i] = functions[i];
    }
    return 1;
  }
  return 0;
}

#ifdef __ARDUINO_X86__
/* sends an interest packet to all the NDN nodes (simulated multicast) */
void NDNOverUDP::multicastSendInterest(NDNInterestPacket *pkt) {
  unsigned long ip, nonce;
  unsigned short nameLength;
  // already in network byte order
  ip = Ethernet.localIP()._sin.sin_addr.s_addr;
  nonce = htonl(pkt->nonce);
  nameLength = htons(pkt->nameLength);
  for (int i = 0; i < _numOfNodes; i++) {
    _udpIstance.beginPacket(_nodes[i], NDN_PORT);
    /* send the actual packet */
    _udpIstance.write((byte *)&ip, sizeof(unsigned long));
    _udpIstance.write((byte *)&(pkt->type), 1);
    _udpIstance.write((byte *)&nonce, sizeof(unsigned long));
    _udpIstance.write((byte *)&nameLength, sizeof(unsigned short));
    _udpIstance.write((byte *)pkt->name, sizeof(char) * pkt->nameLength);
    _udpIstance.endPacket();
  }
}
#else

/* sends an interest packet to all the NDN nodes (broadcast) */
void NDNOverUDP::broadcastSendInterest(NDNInterestPacket *pkt) {
  unsigned long nonce = htonl(pkt->nonce);
  unsigned short nameLength = htons(pkt->nameLength);
  _udpIstance.beginPacket(IPAddress(255, 255, 255, 255), NDN_PORT);
  _udpIstance.write((byte *)&(pkt->type), 1);
  _udpIstance.write((byte *)&nonce, sizeof(unsigned long));
  _udpIstance.write((byte *)&nameLength, sizeof(unsigned short));
  _udpIstance.write((byte *)pkt->name, sizeof(char) * pkt->nameLength);
  _udpIstance.endPacket();
}
#endif

void NDNOverUDP::sendInterest(NDNInterestPacket *packet) {
#ifdef __ARDUINO_X86__
  multicastSendInterest(packet);
#else
  broadcastSendInterest(packet);
#endif
}

void NDNOverUDP::sendData(IPAddress ipDest, char *name,
                          unsigned short nameLength, char *content,
                          unsigned long contentLength) {
  NDNDataPacket dataPkt;
  unsigned long netlong;
  unsigned short netshort;
  dataPkt.type = NDN_DATA_PACKET;
  dataPkt.nameLength = nameLength;
  dataPkt.name = name;
  dataPkt.contentLength = contentLength;
  dataPkt.content = content;

  /* send the actual packet */
  _udpIstance.beginPacket(ipDest, NDN_PORT);
#ifdef __ARDUINO_X86__
  // already in network byte order
  dataPkt.ip = Ethernet.localIP()._sin.sin_addr.s_addr;
  _udpIstance.write((byte *)&(dataPkt.ip), sizeof(unsigned long));
#endif
  _udpIstance.write((byte *)&(dataPkt.type), 1);
  netshort = htons(dataPkt.nameLength);
  _udpIstance.write((byte *)&netshort, sizeof(unsigned short));
  netlong = htonl(dataPkt.contentLength);
  _udpIstance.write((byte *)&netlong, sizeof(unsigned long));
  _udpIstance.write((byte *)dataPkt.name, nameLength);
  _udpIstance.write((byte *)dataPkt.content, contentLength);
  _udpIstance.endPacket();
}

void NDNOverUDP::sendData(IPAddress ipDest, NDNDataPacket *dataPkt) {
  unsigned long netlong;
  unsigned short netshort;
  /* send the packet */
  _udpIstance.beginPacket(ipDest, NDN_PORT);
#ifdef __ARDUINO_X86__
  // already in network byte order
  dataPkt->ip = Ethernet.localIP()._sin.sin_addr.s_addr;
  _udpIstance.write((byte *)&(dataPkt->ip), sizeof(unsigned long));
#endif
  _udpIstance.write((byte *)&(dataPkt->type), 1);
  netshort = htons(dataPkt->nameLength);
  _udpIstance.write((byte *)&netshort, sizeof(unsigned short));
  netlong = htonl(dataPkt->contentLength);
  _udpIstance.write((byte *)&netlong, sizeof(unsigned long));
  _udpIstance.write((byte *)dataPkt->name, dataPkt->nameLength);
  _udpIstance.write((byte *)dataPkt->content, dataPkt->contentLength);
  _udpIstance.endPacket();
}

void NDNOverUDP::receiveData(char *packetBuffer, NDNDataPacket *dataPkt) {
  /* process the packet header*/
  memcpy((void *)&(dataPkt->nameLength), (void *)packetBuffer,
         sizeof(unsigned short));
  dataPkt->nameLength = ntohs(dataPkt->nameLength);
  packetBuffer += sizeof(unsigned short);
  memcpy((void *)&(dataPkt->contentLength), (void *)packetBuffer,
         sizeof(unsigned long));
  dataPkt->contentLength = ntohl(dataPkt->contentLength);
  packetBuffer += sizeof(unsigned long);

  /* payload */
  dataPkt->name = new char[dataPkt->nameLength];
  memcpy((void *)dataPkt->name, (void *)packetBuffer, dataPkt->nameLength);
  packetBuffer += dataPkt->nameLength;
  dataPkt->content = (char *)new byte[dataPkt->contentLength];
  memcpy((void *)dataPkt->content, (void *)packetBuffer,
         dataPkt->contentLength);
}

void NDNOverUDP::receiveInterest(char *packetBuffer,
                                 NDNInterestPacket *interestPkt) {
  /* process the packet header */
  memcpy((void *)&(interestPkt->nonce), (void *)packetBuffer,
         sizeof(unsigned long));
  interestPkt->nonce = ntohl(interestPkt->nonce);
  packetBuffer += sizeof(unsigned long);
  memcpy((void *)&(interestPkt->nameLength), (void *)packetBuffer,
         sizeof(unsigned short));
  interestPkt->nameLength = ntohs(interestPkt->nameLength);
  packetBuffer += sizeof(unsigned short);
  /* payload */
  interestPkt->name = new char[interestPkt->nameLength];
  memcpy((void *)(interestPkt->name), (void *)packetBuffer,
         interestPkt->nameLength);
}

void NDNOverUDP::freeInterestPacket(NDNInterestPacket *pkt) {
  delete[] pkt->name;
}

void NDNOverUDP::freeDataPacket(NDNDataPacket *pkt) {
  delete[] pkt->name;
  delete[] pkt->content;
}

void NDNOverUDP::dumpInterestPacket(NDNInterestPacket *pkt) {
  char buffer[pkt->nameLength + 1];
  /* header */
  Serial.println("Interest Packet");
#ifdef __ARDUINO_X86__
  Serial.print("Interest sender: ");
  Serial.println(IPAddress(pkt->ip));
#endif
  Serial.print("\tNonce: 0x");
  Serial.println(pkt->nonce, HEX);
  Serial.print("\tNameLength: ");
  Serial.println(pkt->nameLength);

  /* payload */
  Serial.print("\tName: ");
  strncpy(buffer, pkt->name, pkt->nameLength);
  buffer[pkt->nameLength] = '\0';
  Serial.print(buffer);
  Serial.print("\n");
}

void NDNOverUDP::dumpDataPacket(NDNDataPacket *pkt) {
  char buffer[pkt->nameLength + 1];
  /* header */
  Serial.println("Data Packet");
#ifdef __ARDUINO_X86__
  Serial.print("Data sender: ");
  Serial.println(IPAddress(pkt->ip));
#endif
  Serial.print("\tNameLength: ");
  Serial.println(pkt->nameLength);
  Serial.print("\tContentLenght: ");
  Serial.println(pkt->contentLength);

  /* payload */
  Serial.print("\tName: ");
  strncpy(buffer, pkt->name, pkt->nameLength);
  buffer[pkt->nameLength] = '\0';
  Serial.print(buffer);
  Serial.print("\n");
  Serial.print("\tContent: ");
  hexDump(pkt->content, pkt->contentLength);
}

void NDNOverUDP::hexDumpInterestPacket(NDNInterestPacket *pkt) {
  Serial.println("HEXDUMP of Interest Packet");
  hexDump((char *)pkt, NDN_HEADER_SIZE_INTEREST);
  hexDump(pkt->name, pkt->nameLength);
  Serial.print("\n");
}

void NDNOverUDP::hexDumpDataPacket(NDNDataPacket *pkt) {
  Serial.println("HEXDUMP of Data Packet");
  hexDump((char *)pkt, NDN_HEADER_SIZE_DATA);
  hexDump(pkt->name, pkt->nameLength);
  hexDump(pkt->content, pkt->contentLength);
  Serial.print("\n");
}

void NDNOverUDP::hexDump(char buffer[], int n) {
  for (int i = 0; i < n; i++) {
    Serial.print("0x");
    Serial.print((byte) * (buffer + i), HEX);
    Serial.print(" ");
  }
  Serial.print("\n");
}

/* NDN Routing Table functions */
bool NDNOverUDP::inRoutingTable(char *name, uint16_t nameLength,
                                unsigned long nonce) {
  uint8_t nameHash[NDN_ROUTING_HASH_SIZE];
  spritz_hash(nameHash, NDN_ROUTING_HASH_SIZE, (uint8_t *)name, nameLength);
  for (int i = 0; i < _routingTableSize; i++) {
    // match
    if (!_routingTable[i].freeBlock &&
        (!spritz_compare(_routingTable[i].interestHash, nameHash,
                         NDN_ROUTING_HASH_SIZE)) &&
        _routingTable[i].nonce == nonce) {
      return true;
    }
  }
  return false;
}

bool NDNOverUDP::setRoute(NDNInterestPacket *pkt) {
  if (_routingFreeEntryIndex == _routingTableSize &&
      _routingTableSize == NDN_ROUTING_TABLE_SIZE) {
    return false;
  }
  if (inRoutingTable(pkt->name, pkt->nameLength, pkt->nonce)) {
    return false;
  }
  int i;
  spritz_hash(_routingTable[_routingFreeEntryIndex].interestHash,
              NDN_ROUTING_HASH_SIZE, (uint8_t *)pkt->name,
              (uint16_t)pkt->nameLength);
  _routingTable[_routingFreeEntryIndex].freeBlock = false;
  _routingTable[_routingFreeEntryIndex].nonce = pkt->nonce;
#ifdef __ARDUINO_X86__
  _routingTable[_routingFreeEntryIndex].ip = IPAddress(pkt->ip);
#else
  _routingTable[_routingFreeEntryIndex].ip = _udpIstance.remoteIP();
#endif
  _routingTable[_routingFreeEntryIndex].timestamp = millis();
  // set _routingTableSize and _routingFreeEntryIndex
  for (i = 0; i < _routingTableSize; i++) {
    if (_routingTable[i].freeBlock) {
      break;
    }
  }
  if (_routingTableSize != NDN_ROUTING_TABLE_SIZE) {
    // no gaps
    if (i == _routingTableSize) {
      _routingTableSize++;
      _routingFreeEntryIndex = _routingTableSize;
    } else {
      _routingFreeEntryIndex = i; // gap found
    }
  }
  return true;
}

NDNRouteEntry *NDNOverUDP::getRoute(char *name, uint16_t nameLength) {
  uint8_t nameHash[NDN_ROUTING_HASH_SIZE];
  spritz_hash(nameHash, NDN_ROUTING_HASH_SIZE, (uint8_t *)name, nameLength);
  for (int i = 0; i < _routingTableSize; i++) {
    // match
    if (!_routingTable[i].freeBlock &&
        !spritz_compare(_routingTable[i].interestHash, nameHash,
                        NDN_ROUTING_HASH_SIZE)) {
      return &_routingTable[i];
    }
  }
  return NULL;
}

void NDNOverUDP::deleteRoute(char *name, uint16_t nameLength) {
  uint8_t nameHash[NDN_ROUTING_HASH_SIZE];
  spritz_hash(nameHash, NDN_ROUTING_HASH_SIZE, (uint8_t *)name, nameLength);
  for (int i = 0; i < _routingTableSize; i++) {
    // match
    if (!_routingTable[i].freeBlock &&
        !spritz_compare(_routingTable[i].interestHash, nameHash,
                        NDN_ROUTING_HASH_SIZE)) {
      _routingTable[i].freeBlock = true;
      if (i == (_routingTableSize - 1)) {
        _routingFreeEntryIndex = i;
        _routingTableSize--;
      } else {
        _routingFreeEntryIndex = i; // gap found
      }
      return;
    }
  }
}

void NDNOverUDP::dropExpiredInterest() {
  int lastFreeBlock = -1;
  if (_routingTableSize > 0) {
    for (int i = _routingTableSize - 1; i > -1; i--) {
      if (!_routingTable[i].freeBlock &&
          (millis() - _routingTable[i].timestamp) > NDN_ROUTING_TTL) {
        _routingTable[i].freeBlock = true;
        _routingFreeEntryIndex = i;
        if (i > lastFreeBlock) {
          lastFreeBlock = i;
        }
      }
    }
    _routingTableSize =
        (lastFreeBlock != -1) ? lastFreeBlock : _routingTableSize;
  }
}

void NDNOverUDP::dumpRoutingTable() {
  Serial.println("Routing Table");
  Serial.print("\tSize: ");
  Serial.println(_routingTableSize);
  Serial.print("\tNext free index: ");
  Serial.println(_routingFreeEntryIndex);
  for (int i = 0; i < _routingTableSize; i++) {
    if (!_routingTable[i].freeBlock) {
      Serial.print("Entry: ");
      Serial.println(i);

      Serial.print("\tFree block: ");
      Serial.println(_routingTable[i].freeBlock);

      Serial.print("\tNonce: ");
      Serial.println(_routingTable[i].nonce, HEX);

      Serial.print("\tInterestHash: ");
      Serial.print("0x");
      for (int k = 0; k < NDN_ROUTING_HASH_SIZE; k++) {
        Serial.print(_routingTable[i].interestHash[k], HEX);
      }
      Serial.print("\n");

      Serial.print("\tIP: ");
      Serial.println(_routingTable[i].ip);

      Serial.print("\tTimestamp: ");
      Serial.println(_routingTable[i].timestamp);

      Serial.println("------------------");
    }
  }
}

void NDNOverUDP::startDaemon() {
  Serial.print("NDN Daemon Listening on IP: ");
  Serial.println(Ethernet.localIP());
  while (1) {
    int packetSize = _udpIstance.parsePacket();
    if (packetSize) {
      int readBytes = _udpIstance.read(_packetBuffer, UDP_BUFFER_SIZE);
      Serial.print("Received ");
      Serial.print(packetSize);
      Serial.print(" bytes from ");
#ifdef __ARDUINO_X86__
      unsigned long addr;
      memcpy((void *)&addr, (void *)_packetBuffer, sizeof(unsigned long));
      _packetBuffer += sizeof(unsigned long);
      addr = ntohl(addr);
      _senderAddr = IPAddress(addr);
      Serial.println(_senderAddr);
#else
      Serial.println(_udpIstance.remoteIP());
      // is this a duplicate broadcast packet?
      if (_udpIstance.remoteIP() == Ethernet.localIP()) {
        Serial.println("Dropped self packet");
        continue;
      }
#endif

      if ((byte)*_packetBuffer == NDN_INTEREST_PACKET) {
        char *content;
        unsigned int contentLength;
        NDNInterestPacket interestPkt;
        bool dataProduced = false;
#ifdef __ARDUINO_X86__
        interestPkt.ip = addr;
#endif
        interestPkt.type = NDN_INTEREST_PACKET;
        receiveInterest(_packetBuffer + 1, &interestPkt);
        // dumpInterestPacket(&interestPkt);
        // hexDump(_packetBuffer, readBytes);

        // Produce data if I am the prodcer
        for (int i = 0; i < _numOfInterests; i++) {
          int pubIntLen = strlen(_interests[i]);
          if (interestPkt.nameLength == pubIntLen) {
            if (strncmp(_interests[i], interestPkt.name, pubIntLen) == 0) {
              // match is found
              dataProduced = true;
              contentLength = _interestsFunctions[i](&content);
#ifdef __ARDUINO_X86__
              sendData(_senderAddr, interestPkt.name, interestPkt.nameLength,
                       content, contentLength);
#else
              sendData(_udpIstance.remoteIP(), interestPkt.name,
                       interestPkt.nameLength, content, contentLength);
#endif
              delete[] content;
              break;
            }
          }
        }
        /* otherwise forward */
        if (!dataProduced) {
          if (setRoute(&interestPkt)) {
            sendInterest(&interestPkt);
          } else {
            Serial.println("Packet dropped");
          }
        }

        /* clean up */
        freeInterestPacket(&interestPkt);
      } else if ((byte)*_packetBuffer == NDN_DATA_PACKET) {
        NDNDataPacket dataPkt;
#ifdef __ARDUINO_X86__
        dataPkt.ip = addr;
#endif
        dataPkt.type = NDN_DATA_PACKET;
        receiveData(_packetBuffer + 1, &dataPkt);
        // dumpDataPacket(&dataPkt);
        // hexDump(_packetBuffer, readBytes);

        // Check FIB and either forward or drop
        NDNRouteEntry *route;
        while ((route = getRoute(dataPkt.name, dataPkt.nameLength)) != NULL) {
          sendData(route->ip, &dataPkt);
          deleteRoute(dataPkt.name, dataPkt.nameLength);
          Serial.println("Packet data forwarded");
        }

        /* clean up */
        freeDataPacket(&dataPkt);
      } else {
        Serial.println("Undefined Packet type");
      }
    } else {
      dropExpiredInterest();
    }
  }
}
