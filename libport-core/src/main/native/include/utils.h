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


   some useful tools
   Author: Yan Zhai

*/
#ifndef _LIBPORT_UTILS_H
#define _LIBPORT_UTILS_H

#include <sstream>
#include <iomanip>
#include <memory>
#include <sys/types.h>
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
inline std::unique_ptr<O> make_unique(Args&&... args) {
  return std::unique_ptr<O>(new O(std::forward<Args>(args)...));
}

template <typename O, class T>
inline std::unique_ptr<O> make_unique(T* pointer) {
  return std::unique_ptr<O>(pointer);
}

std::string read_file(const std::string& fpath);
std::string get_myip();



}

}

#endif

