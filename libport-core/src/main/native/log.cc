
#include "log.h"
#include <syslog.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <mutex>

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

static std::once_flag probe_debug_flag;
static bool write_to_file = false;
static FILE* fdebug = NULL;

void probe_debug_log() {
  struct stat sb;
  if (::stat("/var/run/libport-debug", &sb) == 0) {
    if (S_ISDIR(sb.st_mode)) {
      write_to_file = true;
    }
  }
}

void log(const char* msg, ...) {
  va_list args;
  std::call_once(probe_debug_flag, probe_debug_log);
  va_start(args, msg);
  if (write_to_file) {
    fdebug = fopen("/var/run/libport-debug/log", "a");
    if (fdebug) {
      ::vfprintf(fdebug, msg, args);
      fprintf(fdebug, "\n");
      fclose(fdebug);
    }
  } else {
    vsyslog(current_level, msg, args);
  }
  va_end(args);
}

void log_err(const char *msg, ...) {
  va_list args;
  std::call_once(probe_debug_flag, probe_debug_log);
  va_start(args, msg);
  if (write_to_file) {
    fdebug = fopen("/var/run/libport-debug/log", "a");
    if (fdebug) {
      ::vfprintf(fdebug, msg, args);
      fprintf(fdebug, "\n");
      fclose(fdebug);
    }
  } else {
    vsyslog(LOG_ERR, msg, args);
  }
  va_end(args);
}

}
