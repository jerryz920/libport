/*
   The FreeBSD Copyright

   Copyright 1992-2017 The FreeBSD Project. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.  Redistributions in binary
   form must reproduce the above copyright notice, this list of conditions and
   the following disclaimer in the documentation and/or other materials
   provided with the distribution.  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND
   CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   The views and conclusions contained in the software and documentation are
   those of the authors and should not be interpreted as representing official
   policies, either expressed or implied, of the FreeBSD Project.


   some useful tools
   Author: Yan Zhai

*/
#ifndef _LIBPORT_UTILS_H
#define _LIBPORT_UTILS_H

#include <sstream>
#include <iomanip>
#include <memory>
#include <sys/types.h>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "log.h"

namespace latte {
  namespace utils {

typedef struct {} DecType;
typedef struct {} HexType;
typedef decltype(std::dec) ModeType;

inline ModeType&& mode(DecType) {
  return std::dec;
}

inline ModeType&& mode(HexType) {
  return std::hex;
}

template<typename I, typename BaseType=DecType>
inline std::string itoa(I i) {
  std::stringstream ss;
  ss.exceptions(std::stringstream::failbit);
  ss << mode(BaseType()) << std::showbase << i;
  return ss.str();
}


template<typename I>
inline I atoi(const std::string& s) {
  std::stringstream ss(s);
  ss.exceptions(std::stringstream::failbit);
  I result;
  ss >> std::setbase(0) >> result;
  return result;
}

template <typename O, class... Args>
static inline std::unique_ptr<O> make_unique(Args&&... args) {
  return std::unique_ptr<O>(new O(std::forward<Args>(args)...));
}

template <typename O, class T>
static inline std::unique_ptr<O> make_unique(T* pointer) {
  return std::unique_ptr<O>(pointer);
}

std::string read_file(const std::string& fpath);
std::string get_myip();


int unix_stream_conn(const std::string &path);
int unix_stream_listener(const std::string &path);

std::tuple<pid_t, uid_t, gid_t> unix_auth_id(int fd);

constexpr int UNEXPECTED_CLOSE = -65515;
constexpr int PARSE_ERROR = -65516;

int reliable_send(int fd, const char *buffer, int size);
static inline int reliable_send(int fd, const unsigned char *buffer, int size) {
  return reliable_send(fd, (const char*) buffer, size);
}
int reliable_recv(int fd, char *buffer, int size) ;
static inline int reliable_recv(int fd, unsigned char *buffer, int size) {
  return reliable_recv(fd, (char*) buffer, size);
}


/// A dynamic buffer that can be used and ensured to deleted
class Buffer {

  public:
    Buffer() = delete;
    Buffer(const Buffer&) = delete;
    Buffer(Buffer &&o) = delete;

    Buffer(size_t sz): sz_(sz){
      buf_ = new unsigned char[sz];
    }

    ~Buffer() {
      delete[] buf_;
    }

    unsigned char* buf() { return buf_; }
    const unsigned char* buf() const { return buf_; }
    size_t size() const { return sz_; }

  private:
    unsigned char *buf_;
    size_t sz_;
};

static constexpr int64_t MAX_MSGID = (1ll << 60);

uint64_t gen_rand_uint64();


static inline std::string format_id(const std::string& ip, int lo, int hi) {
  std::stringstream ss;
  ss << ip << ":" << lo << "-" << hi;
  return ss.str();
}

static inline std::string format_principal_id(uint64_t pid) {
  return latte::utils::itoa<uint64_t, latte::utils::HexType>(pid);
}

static inline std::string format_system_time() {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  /// ctime string has exact 26 chars, 50 is enough.
  std::string result(50, ' ');
  ctime_r(&t, &result[0]);
  return result;
}

std::string check_proc(uint64_t pid);

static inline std::string format_image_source(const std::string& source_url,
    const std::string& source_rev) {
  std::stringstream image_source_info;
  image_source_info << source_url << "#" << source_rev ;
  return image_source_info.str();
}

static inline std::string format_netaddr(const std::string& ip, int port) {
  std::stringstream res;
  res << ip << ":" << port;
  return res.str();
}

static inline std::string format_netaddr(const std::string& ip, int port_min, int port_max) {
  std::stringstream res;
  res << ip << ":" << port_min << "-" << port_max;
  return res.str();
}


template<typename F>
class Guards {

  public:
    Guards() = delete;
    Guards(const Guards&) = delete;
    Guards(Guards&&) = delete;

    Guards(std::function<F> f):f_(f) {
    }
    ~Guards() {
      f_();
    }

  private:
    std::function<F> f_;

};




}



}

#undef GOOGLE_DISALLOW_EVIL_CONSTRUCTORS
#define GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&);                           \
  void operator=(const TypeName&)

#undef GOOGLE_DISALLOW_IMPLICIT_CONSTRUCTORS
#define GOOGLE_DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                           \
  TypeName(const TypeName&);                            \
  void operator=(const TypeName&)

#endif

