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
#include <mutex>
#include "cpprest/json.h"

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
      myip_(std::move(other.myip_)),
      local_port_lo_(other.local_port_lo_),
      local_port_hi_(other.local_port_hi_),
      persistence_path_(std::move(other.persistence_path_)),
      client_(std::move(other.client_)),
      port_manager_(std::move(other.port_manager_)),
      index_principals_(std::move(other.index_principals_)),
      principals_(std::move(other.principals_)),
      images_(std::move(other.images_)),
      accessors_(std::move(other.accessors_)),
      config_root_(std::move(other.config_root_)) {
    }

    CoreManager& operator =(CoreManager&& other) {
      myip_ = std::move(other.myip_);
      local_port_lo_ = other.local_port_lo_;
      local_port_hi_ = other.local_port_hi_;
      persistence_path_ = std::move(other.persistence_path_);
      client_ = std::move(other.client_);
      port_manager_ = std::move(other.port_manager_);
      index_principals_ = std::move(other.index_principals_);
      principals_ = std::move(other.principals_);
      images_ = std::move(other.images_);
      accessors_ = std::move(other.accessors_);
      config_root_ = std::move(other.config_root_);
      return *this;
    }

    CoreManager(const std::string& metadata_url, const std::string& myip,
        const std::string& persistence_path);

    /// operations, we may want to use this as "build_image"
    int create_image(
        const std::string& image_hash,
        const std::string& source_url,
        const std::string& source_rev,
        const std::string& misc_conf
        ) noexcept;

    int create_principal(uint64_t id, const std::string& image_hash,
        const std::string& configs, int nport) noexcept;

    int post_object_acl(const std::string& obj_id,
        const std::string& requirements) noexcept;

    int endorse_image(const std::string& image_hash,
        const std::string& endorsement) noexcept;

    bool attest_principal_property(const std::string& ip, uint32_t port,
        const std::string& prop) noexcept;

    bool attest_principal_access(const std::string& ip, uint32_t port,
        const std::string& obj) noexcept;

    int delete_principal(uint64_t uuid) noexcept;

    void save(const std::string& fpath) noexcept;
    static CoreManager* from_disk(const std::string& fpath,
        const std::string &server_url, const std::string &myip);


    // void delete_image();
    // void delete_principal();
    inline const std::string& myip() const { return myip_; }
    inline uint32_t lo() const { return local_port_lo_; }
    inline uint32_t hi() const { return local_port_hi_; }

    /// entrance of syncing thread
    void start_sync_thread();

  private:

    inline void set_new_config_root(web::json::value&& new_root) {
      config_root_ = std::move(new_root);
    }
    /// trying to sync with the config root about principals, images, and objects
    void sync() noexcept ;

    void sync_principals();
    void sync_images();
    void sync_objects();

    void notify_created(std::string&& type, std::string&& key, web::json::value v);
    void notify_deleted(std::string&& type, std::string&& key);

    /// access key in the config root of json
    constexpr static const char* K_PRINCIPAL = "principals";
    constexpr static const char* K_IMAGE = "images";
    constexpr static const char* K_ACCESSOR = "accessors";
    constexpr static int SYNC_DURATION = 30; // second

    std::string myip_;
    uint32_t local_port_lo_;
    uint32_t local_port_hi_;
    std::string persistence_path_;
    std::unique_ptr<MetadataServiceClient> client_;
    std::unique_ptr<PortManager> port_manager_;
    std::map<uint32_t, std::weak_ptr<Principal>> index_principals_;
    std::unordered_map<uint64_t, std::shared_ptr<Principal>> principals_;
    std::unordered_map<std::string, std::unique_ptr<Image>> images_;
    std::unordered_map<std::string, std::unique_ptr<AccessorObject>> accessors_;
    std::mutex write_lock_;
    // the queue with on the fly syncing content
    std::vector< std::tuple<std::string, std::string, bool, web::json::value> > sync_q_;
    std::condition_variable sync_cond_;
    web::json::value config_root_; // root of json object that does sync
};

}


#endif

