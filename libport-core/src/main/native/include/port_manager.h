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

   The internal implementation of the port and ID manager class
   Author: Yan Zhai

*/
#ifndef _PORT_MANAGER_H
#define _PORT_MANAGER_H

#include <cstdint>
#include <utility>
#include <map>
#include <sstream>
#include <algorithm>

namespace latte {

class PortManager {

  public:
    typedef std::pair<uint32_t, uint32_t> PortPair;

    constexpr static uint32_t BadPort = (uint32_t) -1;
    constexpr static std::pair<uint32_t, uint32_t> Bad = std::make_pair(0, 0);

    PortManager(const PortManager&) = delete;
    PortManager& operator =(const PortManager&) = delete;
    PortManager(): PortManager(1, 65535) {}
    PortManager(uint32_t low, uint32_t high): low_(low), high_(high) {
      free_map_[low] = high;
    }
    inline uint32_t low() const { return low_; }
    inline uint32_t high() const { return high_; }

    /// Return [l,r) pair
    inline PortPair allocate(uint32_t cnt) {
      constexpr PortPair Bad;
      PortPair index = find_fit(cnt);
      if (index == Bad) {
        throw_full();
      }
      uint32_t available = index.second - index.first;
      uint32_t remain = available - cnt;
      if (remain >= 1) {
        /// Set in-place would usually avoid reallocating internal node
        free_map_[index.first] = index.first + remain;
      } else {
        free_map_.erase(index.first);
      }
      allocated_[index.first + remain] = index.second;
      return std::make_pair(index.first + remain, index.second);
    }

    /// Allocate exact pair from within the range
    inline PortPair allocate(uint32_t lo, uint32_t hi) {
      if (lo < low() || hi > high()) {
        throw_bad_range(lo, hi);
      } 
      if (is_allocated(lo, hi)) {
        throw_bad_range(lo, hi);
      }
      PortPair p = find_fit(lo, hi);

      // adjust left boundary
      if (p.first < lo) {
        free_map_[p.first] = lo;
      } else {
        free_map_.erase(p.first);
      }
      // adjust right boundary
      if (p.second > hi) {
        free_map_[hi] = p.second;
      }
      // allocate the range
      allocated_[lo] = hi;
      return PortPair(lo, hi);
    }

    // Using any port in range should deallocate it, but we want to make
    // sure it is programmed correctly
    inline void deallocate(uint32_t p) {
      auto allocate_index = allocated_.find(p);
      if (allocate_index == allocated_.end()) {
        throw_bad_port(p);
      }
      auto free_index = free_map_.lower_bound(p);
      /// merge right
      if (free_index != free_map_.end()) {
        auto right_index = free_index;
        if (right_index->first == allocate_index->second) {
          allocate_index->second = right_index->second;
          free_map_.erase(right_index);
        }
      }
      if (free_index != free_map_.begin()) {
        auto left_index = std::prev(free_index);
        if (left_index->second == allocate_index->first) {
          // merge in place
          left_index->second = allocate_index->second;
        } else {
          free_map_[allocate_index->first] = allocate_index->second;
        }
      } else {
        free_map_[allocate_index->first] = allocate_index->second;
      }
      allocated_.erase(allocate_index);
    }

    inline bool is_allocated(uint32_t p) const {
      if (allocated_.size() == 0) {
        return false;
      }
      // order: p < index.first < index.second
      auto index = allocated_.upper_bound(p);
      // if index is already the first, then p is not in allocated map for sure
      if (index == allocated_.cbegin()) {
        return false;
      }
      index = std::prev(index);
      // order: index.first <= p, just need to confirm index.second > p, then p
      // must be included in [index.first, index.second). Note that we don't have
      // overlapped segment, thus if this segment doesn't contain p, no one else will
      return index->second > p;
    }

    /// Test if the range is partially allocated
    inline bool is_allocated(uint32_t l, uint32_t h) const {
      if (allocated_.size() == 0) {
        return false;
      }
      auto index = allocated_.upper_bound(l);

      if (index == allocated_.cbegin()) {
        // if index is the first, then as long as h > index.first it's overlapped
        /// order: l < index.first < index.second
        return index->first < h;
      } else {
        auto index2 = std::prev(index);
        // index2.first <= l < index.first < index.second
        // either index2.second > l (index2 overlaps with [l,r))
        // or index.first < h (index overlaps with [l,r))
        //
        // In other word, if r <= index.first and index2.second <= l, then the
        // whole range of [l,r) is not yet allocated.
        return index2 ->second > l || (index != allocated_.cend() && index->first < h);
      }
    }

    /// Check how many segment there are with size no larger than upto
    inline uint32_t report_fragment(uint32_t upto) const {
      return std::count_if(free_map_.cbegin(), free_map_.cend(),
          [upto](const PortPair &p) { return p.second - p.first <= upto; });
    }

  private:

    inline void throw_full() const {
      throw std::runtime_error("no available port range");
    }

    inline void throw_bug(std::string msg) const {
      std::stringstream ss;
      ss << "a bug occurs:" << msg;
      throw std::runtime_error(ss.str());
    }

    inline void throw_bad_port(uint32_t p) const {
      std::stringstream err_msg;
      err_msg << "port " << p << " is a bad port";
      throw std::runtime_error(err_msg.str());
    }

    inline void throw_bad_range(uint32_t l, uint32_t r) {
      std::stringstream err_msg;
      err_msg << "(" << l << "," << r << ") is a bad range";
      throw std::runtime_error(err_msg.str());
    }

    PortPair find_fit(uint32_t cnt) const {
      constexpr PortPair Bad;
      for (auto i = free_map_.cbegin(); i != free_map_.cend(); ++i) {
        if (cnt <= i->second - i->first) {
          return *i;
        }
      }
      return Bad;
    }

    /// This method is only called after !is_allocated(lo, hi) check, so
    // it is guaranteed to find a segment that fully contains [lo, hi)
    PortPair find_fit(uint32_t lo, uint32_t hi) const {
      constexpr PortPair Bad;
      auto p = free_map_.upper_bound(lo);
      /// p won't be first item because there must be a segment with left
      // edge smaller than lo
      if (p == free_map_.cbegin()) {
        throw_bug("consistency broken: must be a segment smaller than lower bound");
      }
      p = std::prev(p);
      /// p->left <= lo, which is guaranteed to be 
      if (p->second < hi) {
        throw_bug("consistency broken: target segment must include given range");
      }
      return *p;
    }

    /// The port range that is managed by this manager
    uint32_t low_;
    uint32_t high_;
    /// stores [l, r) intervals
    std::map<uint32_t, uint32_t> free_map_;
    std::map<uint32_t, uint32_t> allocated_;
};

}
#endif
