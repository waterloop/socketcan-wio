#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "can.h"

using namespace wlp;

canbus::canbus(const char *iface) : ifname(iface), can_sockfd(-1) {}
canbus::~canbus() {
    if(can_sockfd != -1)
        close(can_sockfd);
}

const char *debug_lvls[] = {"", "ERROR", "WARN", "INFO"};

template<int level>
void canbus::canbus_log(const char *msg) {
    if(level >= 1 && level <= 3 && DEBUG_LEVEL >= level) {
        printf("[canbus] [%s]: %s\n", debug_lvls[level], msg);
    }
}

void canbus::canbus_errno(const char *msg) {
    if(DEBUG_LEVEL >= CANBUS_ERR) {
        printf("[canbus] [ERROR] [syscall] %s: %s\n", msg, strerror(errno));
    }
}

bool canbus::begin() {
    struct ifreq ifr = {};
    struct sockaddr_can addr;

    if((can_sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        canbus_errno("socket open failed");
        return false;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if(ioctl(can_sockfd, SIOCGIFINDEX, &ifr) < 0) {
        canbus_errno("ioctl get interface index failed");
        return false;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if(bind(can_sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        canbus_errno("bind to socket failed");
        return false;
    }

    return true;
}

bool canbus::send(uint32_t id, uint8_t *arr, uint8_t len, bool id_ext) {
    if(len > CAN_MAX_DLEN) {
        canbus_log<CANBUS_ERR>("length > CAN MTU");
        return false;
    }
    if(!check_id(id, id_ext)) {
        canbus_log<CANBUS_ERR>("id invalid");
        return false;
    }

    can_frame frame = {};
    frame.can_id = id;
    if(id_ext) {
        frame.can_id |= CAN_EFF_FLAG;
    }
    frame.can_dlc = len;
    memcpy(frame.data, arr, len);

    int ret = write(can_sockfd, &frame, sizeof(can_frame));
    if(ret < 0) {
        canbus_errno("write failed");
        return false;
    }
    if(ret != sizeof(can_frame)) {
        canbus_log<CANBUS_ERR>("incomplete write");
        return false;
    }
    return true;
}

bool canbus::request(uint32_t id, uint8_t len, bool id_ext) {
    if(len > CAN_MAX_DLEN) {
        canbus_log<CANBUS_ERR>("length > CAN MTU");
        return false;
    }

    if(!check_id(id, id_ext)) {
        canbus_log<CANBUS_ERR>("id invalid");
        return false;
    }

    can_frame frame = {};
    frame.can_id = id | CAN_RTR_FLAG;
    if(id_ext) {
        frame.can_id |= CAN_EFF_FLAG;
    }
    frame.can_dlc = len;

    int ret = write(can_sockfd, &frame, sizeof(can_frame));
    if(ret < 0) {
        canbus_errno("write failed");
        return false;
    }
    if(ret != sizeof(can_frame)) {
        canbus_log<CANBUS_ERR>("incomplete write");
        return false;
    }
    return true;
}

bool canbus::filter(can_filter *filters, size_t nfilters) {
    if(setsockopt(can_sockfd, SOL_CAN_RAW, CAN_RAW_FILTER, filters, nfilters * sizeof(can_filter)) < 0) {
        canbus_errno("setsockopt filter failed");
        return false;
    }
    return true;
}

bool canbus::recv(uint32_t *id, uint8_t *data, uint8_t *len, bool *remote_req, bool *id_ext) {
    can_frame frame;
    int ret = read(can_sockfd, &frame, sizeof(can_frame));
    if(ret < 0) {
        canbus_errno("read failed");
        return false;
    }
    if(ret != sizeof(can_frame)) {
        canbus_log<CANBUS_ERR>("incomplete read");
        return false;
    }

    bool req = frame.can_id & CAN_RTR_FLAG;
    bool ext = frame.can_id & CAN_EFF_FLAG;

    if(id != NULL)
        *id = frame.can_id & CAN_ERR_MASK;

    if(!req && data != NULL)
        memcpy(data, frame.data, frame.can_dlc);

    if(len != NULL)
        *len = frame.can_dlc;

    if(remote_req != NULL)
        *remote_req = req;

    if(id_ext != NULL)
        *id_ext = ext;

    return true;
}

bool canbus::check_id(uint8_t id, bool id_ext) {
    return true;
    if(id_ext) {
        return !(id & ~CAN_EFF_MASK);
    } else {
        return !(id & ~CAN_SFF_MASK);
    }
}
