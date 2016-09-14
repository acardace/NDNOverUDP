
# NDNOverUDP
Named Data Networking (NDN) is a promising
paradigm for the future Internet architecture which opens up
new possibilities for the data exchange among routers.
In order to learn NDN principles, a simpler NDN protocol has
been developed in a mobile environment by the means of different
boards.
This project wants to explore the potentialities of the NDN paradigm applying to a IoT (Internet of Things) context.

## Installing
In order to use the library on Arduino all you have to do is to copy the library folder where you keep all your other Arduino libraries (it usually is ~/Arduino/libraries).

## Library APIs
**NDNOverUDP** has been designed to be as
developers-friendly as possible, the aim was to let programmers easily declare the interests they are able to produce and how they can produce them.
Following this simple schema instantiating a NDN producer
on a local network can be done in few lines.
Here is an example of the only lines of code required to make
an Arduino become a NDN-Forwarder/Router:
```C++
#include <NDNOverUDP.h>
#include <Ethernet.h>

NDNOverUDP ndn;
byte mac[] = { A MAC address };

void setup() {
  ndn.begin(mac);
}
void loop () {
  ndn.startDaemon();
}
```
Here is another example where we would like the Arduino to
become a NDN-Forwarder/Router and a producer as well.
This can be done publishing the interest we are able to
produce.
```C++
#include <NDNOverUDP.h>
#include <Ethernet.h>

NDNOverUDP ndn;
byte mac[] = { A MAC address };

int homeTemp(char **buf) {
  *buf = new char[20];
  return sprintf(*buf, readTempSensor())+1;
}

char *names[20] = { "/home/temp" };
dataProducer funcs[] = { homeTemp };

void setup() {
  ndn.begin(mac);
  ndn.publishInterests(names, funcs, 1);
}

void loop() {
  ndn.startDaemon();
}
```
## Improvement and Collaboration
The scope of the project, which was implementing the
NDN protocol over UDP for small and local IoT applications,
imposes a limitation, the library as it is now cannot be used
over the Internet, in fact it deals only within a local network,
moreover it has been tested only with very few devices,
therefore there’s no claim that this system is scalable with
more than 10-20 devices although the whole project has been
designed and developed with scalability in mind.
NDNOverUDP has got very much room for improvements,
here are few ideas for further development:
* implement security and encryption;
* extend the implementation to support IPv6;
* implement NAT traversal;
* add optional-fields to Interest and Data packets;
* implement Directed-Diffusion routing;

Although there is plenty of room for improvements, since this is a proof of concept, the library is ready for local contexts and easy to use for developers wishing to integrate NDN protocol in their local networks, anyone wishing to contribute is more than welcome!

------------------------------------------------------------
Copyright (C) **Antonio Cardace** and **Davide Aguiari** 2016, antonio@cardace.it
