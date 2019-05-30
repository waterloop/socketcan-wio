# SocketCAN

SocketCAN is a set of open source CAN drivers and a networking stack contributed by Volkswagen Research to the Linux kernel. 
Formerly known as Low Level CAN Framework (LLCF).

This package uses `socketcan` drivers and networking stack to provide an abstraction over interacting with CAN.

### Usage

Create and begin the bus:
```cpp
canbus bus("can0");

if (!bus.begin()) {
    return 1;
}
```

Read data from the bus:
```cpp
uint32_t id;
uint8_t data[8];
uint8_t len;
bool req;

if (!bus->recv(&id, data, &len, &req)) {
    return;
}
```

Write data to the bus:
```cpp
uint8_t buf[8];   // this is the buffer where some data is stored already

if (!bus.send(0x006, buf, sizeof(buf))) {
  return 1;
}
```


### Installation
This is a `wio` package and can be installed:
```bash
wio install socketcan-wio@1.0.0 --url github.com/waterloop/socketcan-wio
```
