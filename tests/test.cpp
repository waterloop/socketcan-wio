#include <stdio.h>
#include "can.h"
#include <linux/can.h>

using namespace wlp;

int main() {
	canbus bus("vcan0");
	if(!bus.begin()) {
		return 1;
	}
	uint8_t arr[] = {1, 2, 3};
	if(!bus.send(43 | CAN_RTR_FLAG, arr, 3)) {
		return 1;
	}
	if(!bus.request(43, 3)) {
		return 1;
	}
	uint32_t id;
	uint8_t data[8];
	uint8_t len;
	if(!bus.recv(&id, data, &len)) {
		return 1;
	}

	printf("Recv: %d (%d)\n", id, len);

	return 0;
}
