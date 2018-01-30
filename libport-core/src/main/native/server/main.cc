
#include <jutils/config/simple.h>
#include "manager.h"
#include "server.h"
#include "config.h"

int main(int argc, char **argv) {
  const char *config = "config.txt";
  if (argc > 1) {
    config = argv[1];
  }
  latte::config::load_config(config);
  auto url = latte::config::metadata_service_url();
  latte::init_manager(url);
  auto server = latte::Server(latte::config::local_daemon_path());
  /// no returns
  server.start();
  return 0;
}
