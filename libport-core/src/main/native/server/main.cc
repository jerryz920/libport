
#include <jutils/config/simple.h>
#include "manager.h"
#include "server.h"
#include "config.h"
#include "jutils/fs.h"

int main(int argc, char **argv) {
  const char *config = "config.txt";
  if (argc > 1) {
    config = argv[1];
  }
  latte::config::load_config(config);


  auto url = latte::config::metadata_service_url();
  latte::init_manager(url);
  auto &daemon = latte::config::local_daemon_path();
  if (::is_file_exist(daemon.c_str())) {
    if (::unlink(daemon.c_str())) {
      ::perror("removing old socket:");
      return 1;
    }
  }
  latte::log("attguard starting on %s", daemon.c_str());
  auto server = latte::Server(latte::config::local_daemon_path());
  /// no returns
  server.start();
  return 0;
}
