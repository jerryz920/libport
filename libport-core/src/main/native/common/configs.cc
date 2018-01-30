
#include "jutils/config/simple.h"
#include "config.h"
#include <sstream>

namespace latte {

namespace config {

ConfigItems config_cache_;

void load_config(const char *path) {
  auto &conf = jutils::config::SimpleConfig::create_config(path);
  const std::string *myid = conf.get(MY_ID);
  if (!myid) {
    throw std::runtime_error("myid not configured");
  }
  const std::string *myip = conf.get(MY_IP);
  if (!myip) {
    throw std::runtime_error("myip not configured");
  }
  const std::string *run_as_iaas = conf.get(latte::config::RUN_AS_IAAS);
  if (!run_as_iaas) {
    throw std::runtime_error("run as iaas not configured!\n");
  }
  const std::string *local_ep = conf.get(latte::config::DAEMON_SOCKET_PATH);
  if (!local_ep) {
    /// using default local ep
    config_cache_.local_ep = DAEMON_SOCKET_DEFAULT_PATH;
  } else {
    config_cache_.local_ep = *local_ep;
  }

  const std::string *metadata_ip = conf.get(latte::config::METADATA_SERVICE_IP);
  const std::string *port_str = conf.get(latte::config::METADATA_SERVICE_PORT);

  if (!metadata_ip) {
    throw std::runtime_error(
        "must provide either metadata_url or metadata_ip");
  }
  std::stringstream buf;
  buf << "http://" << *metadata_ip << ":";
  if (!port_str) {
    buf << METADATA_SERVICE_DEFAULT_PORT;
    config_cache_.metadata_port = METADATA_SERVICE_DEFAULT_PORT;
  } else {
    buf << *port_str;
    /// may come with error soon, but it will be detected anyway and cause no harm.
    config_cache_.metadata_port = atoi(port_str->c_str());
  }
  buf << "/";
  config_cache_.metadata_url = buf.str();
  config_cache_.metadata_ip = std::string(*metadata_ip);

  config_cache_.myid = std::string(*myid);
  config_cache_.myip = std::string(*myip);
  config_cache_.run_as_iaas = (run_as_iaas->compare("1") == 0 ||
      run_as_iaas->compare("true") == 0);
}
}

}
