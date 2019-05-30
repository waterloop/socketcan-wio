#ifndef CAN_H
#define CAN_H

#define CANBUS_ERR 1
#define CANBUS_WARN 2
#define CANBUS_INFO 3

// TODO allow DEBUG_LEVEL to be configured outside of library
#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL CANBUS_INFO
#endif

#include <stdint.h>
#include <linux/can.h>

namespace wlp {
class canbus {
 public:
  canbus(const char *ifname);
  ~canbus();

  // Initialize
  bool begin();

  // Send a CAN frame
  bool send(uint32_t id, uint8_t *arr, uint8_t len, bool id_ext = true);

  // Send a CAN remote request
  bool request(uint32_t id, uint8_t len, bool id_ext = true);

  // Receive a CAN frame
  bool recv(uint32_t *id, uint8_t *data = NULL, uint8_t *len = NULL, bool *remote_req = NULL, bool *id_ext = NULL);

  // Set CAN filter
  bool filter(can_filter *filters, size_t nfilters);
 private:
  template<int level>
  void canbus_log(const char *msg);

  void canbus_errno(const char *msg);

  bool check_id(uint32_t id, bool id_ext);

  const char *ifname;
  int can_sockfd;
};
}

#endif
