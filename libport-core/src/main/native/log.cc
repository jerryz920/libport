
#include "log.h"
#include <syslog.h>
#include <stdarg.h>

namespace latte {
int current_level = LOG_NOTICE;

bool is_debug() {
  return current_level <= LOG_DEBUG;
}

int loglevel() {
   return current_level;
}

void setloglevel(int upto) {
  current_level = upto;
}

void log(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  vsyslog(current_level, msg, args);
  va_end(args);
}

void log_err(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vsyslog(LOG_ERR, msg, args);
  va_end(args);
}

}
