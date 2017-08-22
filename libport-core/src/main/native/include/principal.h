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

namespace latte {
class Principal {


  public:
    // We don't want any copy behavior of principals. We can move them though
    Principal(const Principal& other) = delete;
    Principal& operator =(const Principal&) = delete;

    Principal(Principal &&other):
      id_(other.id_), local_lo_(other.local_lo_),
      local_hi_(other.local_hi_), reserved_(std::move(other.reserved_)),
      statements_(std::move(other.statements_)) { }

    Principal& operator =(Principal &&other) {
      id_ = other.id_;
      local_lo_ = other.local_lo_;
      local_hi_ = other.local_hi_;
      reserved_ = std::move(other.reserved_);
      statements_ = std::move(other.statements_);
      return *this;
    }

    /// We use "copy" on the statement strings, just trying to avoid life
    // management between different language runtimes.
    Principal(uint64_t id, int lo, int hi, std::list<std::string> statements):
      id_(id), local_lo_(lo), local_hi_(hi), reserved_(),
      statements_(statements.begin(), statements.end()) { }

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

    inline void add_reserved(int lo, int hi) {
      reserved_.emplace_back(std::make_pair(lo, hi));
    }

    inline void del_reserved(int lo, int hi) {
      reserved_.remove(std::make_pair(lo, hi));
    }

    inline bool is_reserved(int p) const {
      return find_if(reserved_.cbegin(),
          reserved_.cend(), [p](std::pair<int, int> np) {
             return p <= np.second && p >= np.first;
             }) == reserved_.cend();
    }

    inline const std::list<std::pair<int, int>>& reserved() const {
      return reserved_;
    }

    inline const std::list<std::string>& statements() const {
      return statements_;
    }

    inline void add_statement(const std::string &s) {
      statements_.push_back(s);
    }

    inline void add_statement(std::string &&s) {
      statements_.emplace_back(std::move(s));
    }

  private:
    uint64_t id_;
    int local_lo_;
    int local_hi_;
    std::list<std::pair<int, int>> reserved_;
    std::list<std::string> statements_;
};
}
#endif
