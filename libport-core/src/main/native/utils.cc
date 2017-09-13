
#include "utils.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <errno.h>
#include <string.h>

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
}
}
