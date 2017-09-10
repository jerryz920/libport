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
   CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
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


   Principal definition
   Author: Yan Zhai

*/

#ifndef _LIBPORT_PRINCIPAL_H
#define _LIBPORT_PRINCIPAL_H

#include <list>
#include <utility>
#include <cstdint>
#include <string>
#include <algorithm>
#include "cpprest/json.h"

namespace latte {
class Principal {


  public:
    // We don't want any copy behavior of principals. We can move them though
    Principal(const Principal& other) = delete;
    Principal& operator =(const Principal&) = delete;

    Principal(Principal &&other):
      id_(other.id_), local_lo_(other.local_lo_),
      local_hi_(other.local_hi_), image_(std::move(other.image_)),
      configs_(std::move(other.configs_)) {}

    Principal& operator =(Principal &&other) {
      id_ = other.id_;
      local_lo_ = other.local_lo_;
      local_hi_ = other.local_hi_;
      image_ = std::move(other.image_);
      configs_ = std::move(other.configs_);
      return *this;
    }

    Principal(uint64_t id, int lo, int hi, const std::string& image,
        const std::string& configs=""):
      id_(id), local_lo_(lo), local_hi_(hi), image_(image),
      configs_(configs){}

    inline uint64_t id() const { return id_; }
    inline int lo() const { return local_lo_; }
    inline int hi() const { return local_hi_; }

    inline void set_local(int lo, int hi) {
      if (lo > local_lo_) {
        local_lo_ = lo;
      }
      if (hi < local_hi_) {
        local_hi_ = hi;
      }
    }
    inline const std::string& image() const {
      return image_;
    }

    inline const std::string& configs() const {
      return configs_;
    }

    web::json::value to_json() const;
    static Principal from_json(web::json::value v);
    static Principal parse(const std::string &data);


  private:
    /// Note we don't include IP address because it should be determined
    // at runtime.
    uint64_t id_;
    int local_lo_;
    int local_hi_;
    std::string image_;
    std::string configs_; // configs is properly escaped
};
}
#endif
