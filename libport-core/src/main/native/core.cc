
#include "libport.h"
#include "port-syscall.h"

#include <unordered_map>
using std::unordered_map;


/// Singleton instance of library storage




int libport_init(const char *metadata_host, const char *metadata_service,
    int port_per_principal) {
  return 0;
}

int create_principal(uint64_t uuid, ...) {
  return 0;
}

int speak_for(uint64_t uuid, ...) {
  return 0;
}

int delete_principal(uint64_t uuid) {
  return 0;
}
