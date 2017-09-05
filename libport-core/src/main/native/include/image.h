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


   Image definition
   Author: Yan Zhai

*/

#ifndef _LIBPORT_IMAGE_H
#define _LIBPORT_IMAGE_H

#include <string>
#include "cpprest/json.h"

namespace latte {

class Image {

  public:

    Image() = delete;
    Image(const Image& image) = delete;
    Image& operator =(const Image& other) = delete;

    Image(Image&& other): hash_(std::move(other.hash_)),
      url_(std::move(other.url_)), revision_(std::move(other.revision_)),
      configs_(std::move(other.configs_)){}
    Image(const std::string &hash, const std::string &url,
        const std::string &revision, const std::string &configs=""):
      hash_(hash), url_(url),
      revision_(revision), configs_(configs){}


    Image& operator =(Image&& other) {
      hash_ = std::move(other.hash_);
      url_ = std::move(other.url_);
      revision_ = std::move(other.revision_);
      configs_ = std::move(other.configs_);
      return *this;
    }

    inline const std::string& hash() const {
      return hash_;
    }

    inline const std::string& url() const {
      return url_;
    }

    inline const std::string& revision() const {
      return revision_;
    }

    inline const std::string& configs() const {
      return configs_;
    }

    web::json::value to_json() const;
    static Image parse(const std::string &data);
  private:

    std::string hash_;
    std::string url_;
    std::string revision_;
    std::string configs_;
};

}

#endif

