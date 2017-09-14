
#include "utils.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <errno.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace latte {
  namespace utils {
std::string read_file(const std::string& fpath) {
  struct stat s;
  /// We use c style but we should be good
  if (stat(fpath.c_str(), &s) < 0) {
    log_err("read file %s, lstat: %s", fpath.c_str(), strerror(errno));
    return "";
  }
  std::FILE* f = fopen(fpath.c_str(), "r");
  if (f) {
    std::string result(s.st_size, ' ');
    std::fread(&result[0], s.st_size, 1, f);
    std::fclose(f);
    return result;
  } else {
    log_err("read file %s, open: %s", fpath.c_str(), strerror(errno));
    return "";
  }
}

std::string get_myip() {

  struct ifaddrs *addrs;
  if (getifaddrs(&addrs)) {
    throw std::runtime_error(strerror(errno));
  }

  for (struct ifaddrs *cur = addrs; cur != NULL; cur = cur->ifa_next) {
    /// filter the loopback interface
    if (strcmp(cur->ifa_name, "lo") == 0) {
      continue;
    }
    /// We don't use IPv6 at the moment
    if (cur->ifa_addr->sa_family != AF_INET) {
      continue;
    }
    /// format IP
    char ipbuf[INET_ADDRSTRLEN + 1];
    if (inet_ntop(AF_INET, &((struct sockaddr_in*)cur->ifa_addr)->sin_addr,
        ipbuf, INET_ADDRSTRLEN)) {
      freeifaddrs(addrs);
      return std::string(ipbuf);
    } else {
      freeifaddrs(addrs);
      throw std::runtime_error(strerror(errno));
    }
  }
  freeifaddrs(addrs);
  return "";
}
}
}
