
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <random>
#include <string>
#include <errno.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "utils.h"

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

std::string get_myip() {

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
    if (inet_ntop(AF_INET, &((struct sockaddr_in*)cur->ifa_addr)->sin_addr,
        ipbuf, INET_ADDRSTRLEN)) {
      freeifaddrs(addrs);
      return std::string(ipbuf);
    } else {
      freeifaddrs(addrs);
      throw std::runtime_error(strerror(errno));
    }
  }
  freeifaddrs(addrs);
  return "";
}

int unix_stream_conn(const std::string &path) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    log_err("create unix socket: %s", strerror(errno));
    return fd;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  if (path.size() >= sizeof(addr.sun_path)) {
    log_err("create unix socket: path too long, %s", path.c_str());
    close(fd);
    return -1;
  }
  strncpy(addr.sun_path, path.c_str(), path.size());
  // just to make sure
  addr.sun_path[path.size()] = '\0';

  if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    log_err("connect unix socket to %s: %s", path.c_str(), strerror(errno));
    close(fd);
    return -1;
  }

  return fd;
}

int unix_stream_listener(const std::string &path) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    log_err("create unix socket: %s", strerror(errno));
    return fd;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  if (path.size() >= sizeof(addr.sun_path)) {
    log_err("create unix socket: path too long, %s", path.c_str());
    close(fd);
    return -1;
  }
  strncpy(addr.sun_path, path.c_str(), path.size());
  // just to make sure
  addr.sun_path[path.size()] = '\0';

  if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    log_err("bind unix socket to %s: %s", path.c_str(), strerror(errno));
    close(fd);
    return -1;
  }

  if (listen(fd, 100) < 0) {
    log_err("listen unix socket on %s: %s", path.c_str(), strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

std::tuple<pid_t, uid_t, gid_t> unix_auth_id(int fd) {
  struct ucred cred;
  socklen_t size = sizeof(cred);
  if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &size) < 0) {
    log_err("getsockopt for unix permission: %s", strerror(errno));
    /// for safety, don't set uid,gid to 0 (root)
    return std::make_tuple(0, -1, -1);
  }
  return std::make_tuple(cred.pid, cred.uid, cred.gid);
}

int reliable_send(int fd, const char *buffer, int size) {
  while (size > 0) {
    auto ret = send(fd, buffer, size, MSG_NOSIGNAL);
    if (ret < 0) {
      if (errno == EAGAIN) {
        continue;
      }
      return errno;
    } else if (ret == 0) {
      /// other side closes
      return UNEXPECTED_CLOSE;
    } else {
      buffer += ret;
      size -= ret;
    }
  }
  return 0;
}

int reliable_recv(int fd, char *buffer, int size) {
  while (size > 0) {
    auto ret = recv(fd, buffer, size, 0);
    if (ret < 0) {
      if (errno == EAGAIN) {
        continue;
      }
      return errno;
    } else if (ret == 0) {
      /// other side closes
      return UNEXPECTED_CLOSE;
    } else {
      buffer += ret;
      size -= ret;
    }
  }
  return 0;
}

static std::random_device rd;
static std::mt19937_64 gen(rd());
static std::uniform_int_distribution<int64_t> dist(1, MAX_MSGID/2);

uint64_t gen_rand_uint64() {
  return dist(gen);
}



std::string check_proc(uint64_t pid) {
  std::stringstream exec_path_buf;
  exec_path_buf << "/proc/" << pid << "/exe";
  struct stat s;

  std::string exec_path = exec_path_buf.str();

  if (lstat(exec_path.c_str(), &s) < 0) {
    latte::log_err("diagnose pid %llu, lstat: %s", pid, strerror(errno));
    return "";
  }

  char *execbuf = (char*) malloc(s.st_size + 1);
  if (!execbuf) {
    latte::log_err("diagnose pid %llu, malloc: %s", pid, strerror(errno));
    return "";
  }
  int linksize = readlink(exec_path.c_str(), execbuf, s.st_size + 1);
  if (linksize < 0) {
    latte::log_err("diagnose pid %llu, readlink: %s", pid, strerror(errno));
    free(execbuf);
    return "";
  }
  if (linksize > s.st_size) {
    latte::log_err("diagnose pid %llu, link size changed between stat", pid);
    free(execbuf);
    return "";
  }
  execbuf[linksize] = '\0';
  auto result = std::string(execbuf, execbuf + linksize);
  free(execbuf);
  return result;
}

}
}
