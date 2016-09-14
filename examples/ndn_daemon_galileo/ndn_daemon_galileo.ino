#include <NDNOverUDP.h>
#include <Ethernet.h>

NDNOverUDP ndn;
byte mac[] = { 0x98, 0x4F, 0xEE, 0x00, 0xF1, 0x90 };
IPAddress ip(192, 168, 0, 91);
IPAddress nodes[] = {IPAddress(192, 168, 0, 90)};

int dump(char **buf) {
  *buf = new char[1];
  ndn.dumpRoutingTable();
  return 1;
}

int temp(char **buf) {
  *buf = new char[5];
  return sprintf(*buf, "27 C")+1;
}

char *names[20] = { "/routing/dump", "/home/temp"};
dataProducer funcs[] = { dump , temp};

void setup() {
  Serial.begin(9600);
  system("ifconfig eth0 > /dev/GS0");
  ndn.begin(mac, ip);
  ndn.addNDNNodes(nodes, 1);
  ndn.publishInterests(names, funcs, 2);
}

void loop() {
  ndn.startDaemon();
}
