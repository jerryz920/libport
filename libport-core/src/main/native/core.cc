
#include "libport.h"
#include "port-syscall.h"
#include "metadata.h"
#include "log.h"
#include "core_manager.h"
#include "image.h"
#include "principal.h"
#include "accessor_object.h"
#include "port_manager.h"

#include <unordered_map>
#include <sys/types.h>
#include <ifaddrs.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <cstdlib>
#include <sstream>
using std::unordered_map;


/// Singleton instance of library storage

namespace {
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
    if (!inet_ntop(AF_INET, &((struct sockaddr_in*)cur->ifa_addr)->sin_addr,
        ipbuf, INET_ADDRSTRLEN)) {
      return std::string(ipbuf);
    } else {
      freeifaddrs(addrs);
      throw std::runtime_error(strerror(errno));
    }
  }
  freeifaddrs(addrs);
  return "";
}

std::string format_id(const std::string& ip, int lo, int hi) {
  std::stringstream ss;
  ss << ip << ":" << lo << "-" << hi;
  return ss.str();
}

std::string format_principal_id(uint64_t pid) {
  /// TODO: make this an utility
  std::stringstream ss;
  ss << std::hex << std::showbase << pid;
  return ss.str();
}

}


namespace latte {
  /// Implementation of core manager

  CoreManager::CoreManager(const std::string& metadata_url, std::string&& myip):
    myip_(std::move(myip)) {
    int lo, hi;
    if (get_local_ports(&lo, &hi) != 0) {
      throw std::runtime_error("error fetching ip local ports");
    }
    port_manager_ = std::unique_ptr<PortManager>(
          new PortManager((uint32_t) lo, (uint32_t) hi));
    client_ = std::unique_ptr<MetadataServiceClient>(
        new MetadataServiceClient(metadata_url, format_id(myip_, lo, hi)));
  }

  int CoreManager::create_image(
      const std::string& image_hash,
      const std::string& source_url,
      const std::string& source_rev,
      const std::string& misc_conf
      ) {
    images_[image_hash] = std::unique_ptr<Image>(new Image(image_hash,
          source_url, source_rev, misc_conf));
    try {
      client_->post_new_image(image_hash, source_url, source_rev, misc_conf);
      return 0;
    } catch (std::runtime_error e) {
      return -1;
    }
  }

  int CoreManager::create_principal(uint64_t id,
      const std::string& image_hash, const std::string& configs, int nport) {
    try {
      auto prange = port_manager_->allocate(nport);
      std::shared_ptr<Principal> newp = std::make_shared<Principal>(id,
            prange.first, prange.second, image_hash, configs);
      index_principals_[prange.first] = std::weak_ptr<Principal>(newp);
      principals_[id] = std::move(newp);

      client_->post_new_principal(format_principal_id(id), 
          myip_, prange.first, prange.second, image_hash, configs);
    } catch (std::runtime_error) {
      return -1;
    }
    return 0;
  }

  int CoreManager::post_object_acl(const std::string& obj_id,
      const std::string& requirement) {
    try {
      const auto insert_res = accessors_.emplace(obj_id);
      const auto& obj_ptr = insert_res.first->second;
      obj_ptr->add_acl(requirement);
      client_->post_object_acl(obj_id, requirement);
    } catch(std::runtime_error) {
      return -1;
    }
    return 0;
  }

  int CoreManager::endorse_image(const std::string& image_hash,
      const std::string& endorsement) {
    try {
      if (images_.find(image_hash) == images_.cend()) {
        log_err("Endorsing: can not find image %s", image_hash.c_str());
        return -1;
      }
      client_->endorse_image(image_hash, endorsement);
    } catch(std::runtime_error) {
      return -1;
    }
    return 0;
  }

  bool CoreManager::attest_principal_property(const std::string& ip,
      uint32_t port, const std::string& property) {
    try {
      return client_->has_property(ip, port, property);
    } catch(std::runtime_error) {
      return false;
    }
  }

  bool CoreManager::attest_principal_access(const std::string& ip,
      uint32_t port, const std::string& obj) {
    try {
      return client_->has_property(ip, port, obj);
    } catch(std::runtime_error) {
      return false;
    }
  }

}


static std::unique_ptr<latte::CoreManager> core;

// A list of principals, images and objects maintaned by this library

int libport_init(const char *server_url) {
  try {
    std::string myip = get_myip();
    if (myip.compare("") == 0) {
      latte::log_err("failed to initialize libport principal identity");
      return -1;
    }
    core = std::unique_ptr<latte::CoreManager>(
        new latte::CoreManager(server_url, std::move(myip)));
  } catch (std::runtime_error) {
    return -1;
  }
  return 0;
}

#define CHECK_LIB_INIT \
  if (!core) { \
    latte::log_err("the library is not initialized");\
    return -1; \
  }

#define CHECK_LIB_INIT_BOOL \
  if (!core) { \
    latte::log_err("the library is not initialized");\
    return false; \
  }

int create_principal(uint64_t uuid, const char *image, const char *config,
    int nport) {
  // create principal, addresses are assgined by the library. Any statement
  // appended will be included must be "const char*" type, which means String
  // in Java. It will be included in misc config fields
  CHECK_LIB_INIT;
  latte::log("creating principal %llu, %s, %s, %d\n", uuid, image, config, nport);
  return core->create_principal(uuid, image, config, nport);
}


int create_image(const char *image_hash, const char *source_url,
    const char *source_rev, const char *misc_conf) {
  CHECK_LIB_INIT;
  latte::log("creating image %s, %s, %s, %s\n", image_hash, source_url,
      source_rev, misc_conf);
  return core->create_image(image_hash, source_url, source_rev, misc_conf);
}

int post_object_acl(const char *obj_id, const char *requirement) {
  CHECK_LIB_INIT;
  latte::log("posting obj acl: %s, %s\n", obj_id, requirement);
  return core->post_object_acl(obj_id, requirement);
}

int endorse_image(const char *image_hash, const char *endorsement) {
  CHECK_LIB_INIT;
  latte::log("endorsing image: %s, %s\n", image_hash, endorsement);
  return core->post_object_acl(image_hash, endorsement);
}

bool attest_principal_property(const char *ip, uint32_t port, const char *prop) {
  CHECK_LIB_INIT_BOOL;
  latte::log("attesting property: %s, %u, %s\n", ip, port, prop);
  return core->attest_principal_property(ip, port, prop);
}

bool attest_principal_access(const char *ip, uint32_t port, const char *obj) {
  CHECK_LIB_INIT_BOOL;
  latte::log("attesting access: %s, %u, %s\n", ip, port, obj);
  return core->attest_principal_access(ip, port, obj);
}





