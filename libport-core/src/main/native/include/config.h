
#ifndef _LIBPORT_CONFIG_H
#define _LIBPORT_CONFIG_H

#include <string>
namespace latte {
/// to pass configure we now have a plain string, but we will leave
//space for the map.
constexpr const char *LEGACY_CONFIG_KEY = "LEGACY_CONFIG_STRING";
constexpr const char *DEFAULT_SPEAKER = "default-speaker";

/// For object smaller than such size we will use pre-allocated buffer
// for larger one we dynamic allocate
const constexpr int RECV_BUFSZ = 8192;
const constexpr int SEND_BUFSZ = 8192;
const constexpr int PROTO_MAGIC = 0x1987;


namespace config {

constexpr const char *LOG_LEVEL = "log";
constexpr const char *MY_ID = "speaker_id";
constexpr const char *MY_IP = "speaker_ip";
constexpr const char *RUN_AS_IAAS = "run_as_iaas";
constexpr const char *METADATA_SERVICE_URL = "metadata_url";
constexpr const char *METADATA_SERVICE_IP = "metadata_ip";
constexpr const char *METADATA_SERVICE_PORT = "metadata_port";
constexpr const uint32_t METADATA_SERVICE_DEFAULT_PORT = 7777;
constexpr const char *DAEMON_SOCKET_PATH = "daemon_socket";
constexpr const char *DAEMON_SOCKET_DEFAULT_PATH = "/var/run/latte/guard.sock";



struct ConfigItems {
  std::string myid; /// This must be set to the VM instance ID
  std::string myip;
  std::string metadata_url;
  std::string metadata_ip;
  std::string local_ep;
  uint32_t metadata_port;
  bool run_as_iaas;
} ;

extern ConfigItems config_cache_;

static inline const std::string &myid() {
  return config_cache_.myid;
}

static inline const std::string &myip() {
  return config_cache_.myip;
}

static inline const std::string &metadata_service_url() {
  return config_cache_.metadata_url;
}

static inline const std::string &metadata_service_ip() {
  return config_cache_.metadata_ip;
}

static inline uint32_t metadata_service_port() {
  return config_cache_.metadata_port;
}

static inline bool run_as_iaas() {
  return config_cache_.run_as_iaas;
}

static inline const std::string &local_daemon_path() {
  return config_cache_.local_ep;
}

void load_config(const char *path);


}
}

#endif
