#include <stdio.h>
#include <thread>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "canbus.h"

using namespace wlp;

uint32_t serial;

int get_serial() {
	int fd = open("/dev/vcio", 0);
	if (fd == -1)
	{
		perror("open /dev/vcio");
		exit(EXIT_FAILURE);
	}

	uint32_t property[32] =
	{
		0x00000000,
		0x00000000,
		0x00010004,
		0x00000010,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000
	};

	property[0] = 10 * sizeof(property[0]);

	if (ioctl(fd, _IOWR(100, 0, char *), property) == -1)
	{
		perror("ioctl");
		exit(EXIT_FAILURE);
	}

	close(fd);

	return property[5];
}

void read_thread(canbus *bus) {
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
    bool req;

    while(true) {
        if(!bus->recv(&id, data, &len, &req)) {
            return;
        }
		if(id == 64) {
			uint32_t remote_serial = *(uint32_t *) data;
			printf("Remote serial received: %x\n", remote_serial);
			if(remote_serial < serial) {
				printf("Setting as slave\n");
			}
		}
    }
}

int main() {

	serial = get_serial();

	printf("%x\n", serial);

    canbus bus("can0");
    if(!bus.begin()) {
        return 1;
    }

	std::thread thread(read_thread, &bus);

	while(true) {
		if(!bus.send(64, (uint8_t *) &serial, 4)) {
			return 1;
		}
		usleep(10000);
	}

    return 0;
}
