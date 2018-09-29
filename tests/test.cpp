#include <stdio.h>

#include "can.h"

using namespace wlp;

int main() {
    canbus bus("vcan0");
    if(!bus.begin()) {
        return 1;
    }
    uint8_t arr[] = {1, 2, 3};
    if(!bus.send(43, arr, 3)) {
        return 1;
    }
    if(!bus.request(43, 3)) {
        return 1;
    }
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
    bool req;

    while(true) {
        if(!bus.recv(&id, data, &len, &req)) {
            return 1;
        }
        printf("Recv: %d (%d) ", id, len);
        if(!req) {
            for(uint8_t i = 0; i < len; ++i) {
                printf("%02x ", data[i]);
            }
            printf("\n");
        } else {
            printf("remote request\n");
        }
    }

    return 0;
}
