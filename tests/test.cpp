#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mutex>
#include <thread>

#include <sys/ioctl.h>

#include "canbus.h"

#include <MCP2515.h>
#include <Wlp/Canbus.h>
#include <Wlp/CanbusDriver.h>

using namespace wlp;

uint32_t serial;

volatile MCP2515 bus();

volatile Packet p{};

volatile bool is_slave = true;

volatile uint64_t last_slave_pkt;

std::mutex mut;

int get_serial() {
  int fd = open("/dev/vcio", 0);
  if (fd == -1) {
    perror("open /dev/vcio");
    exit(EXIT_FAILURE);
  }

  uint32_t property[32] = {0x00000000, 0x00000000, 0x00010004, 0x00000010,
                           0x00000000, 0x00000000, 0x00000000, 0x00000000,
                           0x00000000, 0x00000000};

  property[0] = 10 * sizeof(property[0]);

  if (ioctl(fd, _IOWR(100, 0, char *), property) == -1) {
    perror("ioctl");
    exit(EXIT_FAILURE);
  }

  close(fd);

  return property[5];
}

uint64_t get_time() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000LLU + ts.tv_nsec;
}

void timer_thread() {
  while (true) {
    mut.lock();
    uint64_t tdiff = get_time() - last_slave_pkt;
    mut.unlock();
    if (tdiff > 100000000) {
      if (is_slave) {
        is_slave = false;
        printf("Setting as master: packet timed out\n");
      }
    }
    usleep(1000);
  }
}

void read_thread(canbus *bus) {
  uint32_t id;
  uint8_t data[8];
  uint8_t len;
  bool req;

  while (true) {
    if (!bus->recv(&id, data, &len, &req)) {
      return;
    }
    if (id == 64) {
      uint32_t remote_serial = *(uint32_t *)data;
      if (remote_serial < serial) {
        mut.lock();
        last_slave_pkt = get_time();
        mut.unlock();
        if (!is_slave) {
          is_slave = true;
          printf("Setting as slave: lower serial detected: %x\n",
                 remote_serial);
        }
      }
    }
  }
}

void send_to_arduino(MCP2515 &bus, Packet &p) {
  uint8_t buffer = packet::send(bus, p);
  if (buffer) {
    printf("Sent packet to Arduino");
    return;
  }
  printf("Packet missed");
}

int main() {
  serial = get_serial();

  printf("%x\n", serial);

  canbus bus("can0");
  if (!bus.begin()) {
    return 1;
  }

  printf("Starting as slave\n");

  mut.lock();
  last_slave_pkt = get_time();
  mut.unlock();
  std::thread thread(read_thread, &bus);
  std::thread tthread(timer_thread);

  while (true) {
    if (!bus.send(64, (uint8_t *)&serial, 4)) {
      return 1;
    }
    send_to_arduino(bus, p);
    usleep(10000);
  }

  return 0;
}
