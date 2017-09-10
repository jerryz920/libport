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

#ifndef _CORE_MANAGER_H
#define _CORE_MANAGER_H

#include <memory>
#include <unordered_map>
#include <map>
#include <string>
#include <sstream>

namespace latte {

class Principal;
class AccessorObject;
class Image;
class MetadataServiceClient;
class PortManager;


class CoreManager {

  public:
    CoreManager() = delete;
    CoreManager(const CoreManager& other) = delete;
    CoreManager& operator =(const CoreManager& other) = delete;
    CoreManager(CoreManager&& other):
      client_(std::move(other.client_)),
      port_manager_(std::move(other.port_manager_)),
      principals_(std::move(other.principals_)),
      images_(std::move(other.images_)),
      accessors_(std::move(other.accessors_)){
    }

    CoreManager& operator =(CoreManager&& other) {
      client_ = std::move(other.client_);
      port_manager_ = std::move(other.port_manager_);
      principals_ = std::move(other.principals_);
      images_ = std::move(other.images_);
      accessors_ = std::move(other.accessors_);
      return *this;
    }

    CoreManager(const std::string& metadata_url, std::string&& myid);

    /// operations, we may want to use this as "build_image"
    int create_image(
        const std::string& image_hash,
        const std::string& source_url,
        const std::string& source_rev,
        const std::string& misc_conf
        );

    int create_principal(uint64_t id, const std::string& image_hash,
        const std::string& configs, int nport);

    int post_object_acl(const std::string& obj_id,
        const std::string& requirements);

    int endorse_image(const std::string& image_hash,
        const std::string& endorsement);

    bool attest_principal_property(const std::string& ip, uint32_t port,
        const std::string& prop);

    bool attest_principal_access(const std::string& ip, uint32_t port,
        const std::string& obj);

    // void delete_image();
    // void delete_principal();
    inline const std::string& myip() const { return myip_; }
    inline uint32_t lo() const { return local_port_lo_; }
    inline uint32_t hi() const { return local_port_hi_; }

  private:

    void to_disk(const std::string& path);
    static CoreManager& from_disk(const std::string& path);

    std::string myip_;
    uint32_t local_port_lo_;
    uint32_t local_port_hi_;
    std::unique_ptr<MetadataServiceClient> client_;
    std::unique_ptr<PortManager> port_manager_;
    std::map<uint32_t, std::weak_ptr<Principal>> index_principals_;
    std::unordered_map<uint64_t, std::shared_ptr<Principal>> principals_;
    std::unordered_map<std::string, std::unique_ptr<Image>> images_;
    std::unordered_map<std::string, std::unique_ptr<AccessorObject>> accessors_;
};

}


#endif

