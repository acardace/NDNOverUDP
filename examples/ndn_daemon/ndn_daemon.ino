#include <NDNOverUDP.h>
#include <Ethernet.h>

NDNOverUDP ndn;
byte mac[] = { 0x98, 0x4F, 0xEE, 0x00, 0xF2, 0xA1 };
IPAddress ip(192, 168, 1, 90);


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
  ndn.begin(mac, ip);
  ndn.publishInterests(names, funcs, 2);
}

void loop() {
  ndn.startDaemon();
}
