#include "utils.h"

#include <glog/logging.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace xllm_service {
namespace utils {

bool is_port_available(int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    LOG(ERROR) << "create socket failed.";
    return false;
  }

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    LOG(WARNING) << "set socket options failed.";
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
    return false;
  }
  close(fd);

  return true;
}

} // utils
} // xllm_service