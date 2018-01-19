
#include <jutils/config/simple.h>
#include "manager.h"
#include "server.h"
#include "config.h"

int main() {
  jutils::config::SimpleConfig::create_config("config.txt");
  auto url = latte::config::metadata_service_url();
  latte::init_manager(url);
  auto server = latte::Server(latte::config::local_daemon_path());
  /// no returns
  server.start();
  return 0;
}
