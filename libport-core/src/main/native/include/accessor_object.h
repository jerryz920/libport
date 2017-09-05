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

    Accessor object definition

*/

#ifndef _LIBPORT_ACCESSOR_OBJECT_H
#define _LIBPORT_ACCESSOR_OBJECT_H
#include <string>
#include "cpprest/json.h"
#include <unordered_set>


namespace latte {

  class AccessorObject {

    public:
      AccessorObject() = delete;
      AccessorObject(const AccessorObject&) = delete;
      AccessorObject& operator =(const AccessorObject&) = delete;

      AccessorObject& operator =(AccessorObject &&other) {
        name_ = std::move(other.name_);
        acls_ = std::move(other.acls_);
        return *this;
      }

      AccessorObject(AccessorObject &&other):
        name_(std::move(other.name_)), acls_(std::move(other.acls_)) {}
      AccessorObject(const std::string& name): name_(name) {}

      inline const std::string& name() const { return name_; }
      inline const std::unordered_set<std::string> acls() const { return acls_; }
      inline void add_acl(const std::string &acl) { acls_.insert(acl); }
      inline void del_acl(const std::string &acl) { acls_.erase(acl); }
      inline bool has_acl(const std::string &acl) const {
        return acls_.find(acl) != acls_.cend();
      }

      web::json::value to_json() const;
      static AccessorObject parse(const std::string &data);


    private:
      std::string name_;
      std::unordered_set<std::string> acls_;
  };

}


#endif


