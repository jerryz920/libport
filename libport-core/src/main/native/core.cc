
#include "libport.h"
#include "port-syscall.h"
#include "metadata.h"
#include "log.h"

#include <unordered_map>
#include <sys/types.h>
#include <ifaddrs.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <cstdlib>
#include <sstream>
using std::unordered_map;


/// Singleton instance of library storage

namespace {
std::string get_myid() {

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
    if (!inet_ntop(AF_INET, &((struct sockaddr_in*)cur->ifa_addr)->sin_addr,
        ipbuf, INET_ADDRSTRLEN)) {
      std::stringstream result;
      result << ipbuf << ":";
      int lo, hi;
      if (get_local_ports(&lo, &hi) != 0) {
        freeifaddrs(addrs);
        throw std::runtime_error("error fetching ip local ports");
      }
      result << lo << "-" << hi;
      freeifaddrs(addrs);
      return result.str();
    } else {
      freeifaddrs(addrs);
      throw std::runtime_error(strerror(errno));
    }
  }
  freeifaddrs(addrs);
  return "";
}
}

/// A per-process metadata client. It is used only on "fork" so we should
// be good.
static latte::MetadataServiceClient *client = NULL;
static std::string myid = "";

// A list of principals, images and objects maintaned by this library

int libport_init(const char *metadata_host, const char *metadata_service,
    const char *scheme) {
  auto effective_scheme = scheme;
  if (strcmp(scheme, "") == 0) {
    effective_scheme = "http";
  }
  std::stringstream server_url_buf;
  server_url_buf << effective_scheme << "://" << metadata_host 
             << ":" << metadata_service;
  /// If anything
  std::string server_url = server_url_buf.str();
  latte::log("initializing metadata address: %s", server_url.c_str());
  client = new latte::MetadataServiceClient(server_url);
  if (!client) {
    latte::log_err("failed to initialize libport metadata client");
    return -1;
  }
  myid = get_myid();
  if (myid.compare("") == 0) {
    latte::log_err("failed to initialize libport principal identity");
    return -1;
  }
  latte::log("principal identity: %s", myid.c_str());
  return 0;
}
// create image

int create_principal(uint64_t uuid, const char *image, const char *string, int nport) {
  // create principal, addresses are assgined by the library. Any statement
  // appended will be included must be "const char*" type, which means String
  // in Java. It will be included in misc config fields
  if (client == NULL) {
    latte::log_err("the library is not initialized");
    return -1;
  }

  return 0;
}

int delete_principal(uint64_t uuid) {
  return 0;
}





