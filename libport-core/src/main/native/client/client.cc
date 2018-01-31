


#include "port-syscall.h"
#include "libport.h"
#include "log.h"
#include "utils.h"
#include "safe.h"
#include "config.h"
#include "proto/statement.pb.h"
#include "proto/traits.h"
#include "proto/utils.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "comm.h"
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <memory>
#include <random>




namespace latte {

/// do we have thread safety problem?

/// The guard is unix only, in order to take advantage of unix domain socket
// and authentication mechanism
class AttGuardClient {

  public:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(AttGuardClient);

    AttGuardClient(std::string myid, std::string myip): AttGuardClient(std::move(myid), std::move(myip), DEFAULT_DAEMON_PATH) { }

    AttGuardClient(std::string myid, std::string myip, std::string daemon_path):
        myid_(std::move(myid)), myip_(std::move(myip)), daemon_path_(std::move(daemon_path)), sock_(0) {
      redial();
    }

    void redial() {
      if (sock_) {
        close(sock_);
      }
      sock_ = utils::unix_stream_conn(daemon_path_);
      if (sock_ < 0) {
        throw std::runtime_error("can not connect to the attestation guard, abort");
      }
    }

    ~AttGuardClient() {
      close(sock_);
    }

    const std::string &myip() const { return myip_; }

    //// TODO: optimize the parameters involving constant ref. We don't want to 
    //make extra copy if things can be moved.

    int create_principal(uint64_t uuid, const std::string &image,
        const std::string &config, const std::string &ip, uint32_t lo,
        uint32_t hi) {
      proto::Principal p;
      p.set_id(uuid);
      auto auth = p.mutable_auth();
      auth->set_ip(ip);
      auth->set_port_lo(lo);
      auth->set_port_hi(hi);
      auto code = p.mutable_code();
      code->set_image(image);
      code->set_wildcard(config == "*");
      auto& confmap = *code->mutable_config();
      confmap[LEGACY_CONFIG_KEY] = config;
      return quick_post<proto::Command::CREATE_PRINCIPAL>(p);
    }

    int delete_principal(uint64_t uuid) {
      proto::Principal p;
      p.set_id(uuid);
      return quick_post<proto::Command::DELETE_PRINCIPAL>(p);
    }

    std::unique_ptr<proto::Principal> get_principal(const std::string &ip, uint32_t lo) {
      proto::Principal lookup;
      auto authid = lookup.mutable_auth();
      authid->set_ip(ip);
      authid->set_port_lo(lo);
      return quick_get_principal<proto::Command::GET_PRINCIPAL>(lookup);
    }

    std::unique_ptr<proto::Principal> get_local_principal(uint64_t uuid) {
      proto::Principal lookup;
      lookup.set_id(uuid);
      return quick_get_principal<proto::Command::GET_PRINCIPAL>(lookup);
    }

    std::unique_ptr<proto::MetadataConfig> get_metadata_config() {
      proto::Empty placeholder;
      auto resp = post(prepare<proto::Command::GET_METADATA_CONFIG>(placeholder, myid_.c_str()));
      return resp.extract_metadata_config();
    }

    int endorse_principal(const std::string &ip, uint32_t port, uint64_t gn, int type,
        const std::string &property) {
      proto::EndorsePrincipal statement;
      auto p = statement.mutable_principal();
      p->set_gn(gn);
      auto auth = p->mutable_auth();
      auth->set_ip(ip);
      auth->set_port_lo(port);
      auto code = p->mutable_code();
      auto& confmap = *code->mutable_config();
      confmap[LEGACY_CONFIG_KEY] = "*";
      auto endorsement = statement.add_endorsements();
      set_type(endorsement, type);
      endorsement->set_property(property);
      return quick_post<proto::Command::ENDORSE_PRINCIPAL>(statement);
    }

    int revoke_principal_endorse(const std::string &, uint32_t , int ,
        const std::string &) {
      log_err("Not Implemented: revoke_principal");
      return -1;
    }

    int _endorse(const std::string &id, const std::string &config,
        proto::Endorse::Type stmt_type, int endorse_type, const std::string &property) {
      proto::Endorse statement;
      statement.set_id(id);
      statement.set_type(stmt_type);
      auto endorsement = statement.add_endorsements();
      set_type(endorsement, endorse_type);
      endorsement->set_property(property);
      auto& confmap = *statement.mutable_config();
      confmap[LEGACY_CONFIG_KEY] = config;
      return quick_post<proto::Command::ENDORSE>(statement);
    }


    int endorse_image(const std::string &id, const std::string &config,
        int type, const std::string &property) {
      return _endorse(id, config, proto::Endorse::IMAGE, type, property);
    }

    int endorse_source(const std::string &id, const std::string &config,
        int type, const std::string &property) {
      return _endorse(id, config, proto::Endorse::SOURCE, type, property);
    }

    int revoke(const std::string &, const std::string &, 
        int , const std::string &) {
      log_err("Not Implemented: revoke");
      return -1;
    }

    int endorse_membership(const std::string &ip, uint32_t port, uint64_t gn,
        const std::string &property) {
      proto::EndorsePrincipal statement;
      auto p = statement.mutable_principal();
      p->set_gn(gn);
      auto auth = p->mutable_auth();
      auth->set_ip(ip);
      auth->set_port_lo(port);
      /// We do not need config in principal endorsement...
      auto code = p->mutable_code();
      auto& confmap = *code->mutable_config();
      confmap[LEGACY_CONFIG_KEY] = "*";
      auto endorsement = statement.add_endorsements();
      set_type(endorsement, proto::Endorsement::MEMBERSHIP);
      endorsement->set_property(property);
      return quick_post<proto::Command::ENDORSE_MEMBERSHIP>(statement);
    }

    int endorse_attester(const std::string &id, const std::string &config) {
      proto::Endorse statement;
      statement.set_id(id);
      auto endorsement = statement.add_endorsements();
      endorsement->set_type(proto::Endorsement::ATTESTER);
      auto& confmap = *statement.mutable_config();
      confmap[LEGACY_CONFIG_KEY] = config;
      return quick_post<proto::Command::ENDORSE_ATTESTER_IMAGE>(statement);
    }

    int endorse_builder(const std::string &id, const std::string &config) {
      proto::Endorse statement;
      statement.set_id(id);
      auto endorsement = statement.add_endorsements();
      endorsement->set_type(proto::Endorsement::BUILDER);
      auto& confmap = *statement.mutable_config();
      confmap[LEGACY_CONFIG_KEY] = config;
      return quick_post<proto::Command::ENDORSE_BUILDER_IMAGE>(statement);
    }

    int endorse_source(const std::string &id, const std::string &config,
        const std::string &source) {
      proto::Endorse statement;
      statement.set_id(id);
      auto endorsement = statement.add_endorsements();
      endorsement->set_type(proto::Endorsement::SOURCE);
      endorsement->set_property(source);
      auto& confmap = *statement.mutable_config();
      confmap[LEGACY_CONFIG_KEY] = config;
      return quick_post<proto::Command::ENDORSE_SOURCE_IMAGE>(statement);
    }

    int post_acl(const std::string &id, const std::string &acl) {
      proto::PostACL statement;
      statement.set_name(id);
      statement.add_policies(acl);
      return quick_post<proto::Command::POST_ACL>(statement);
    }

    int check_property(const std::string &ip, uint32_t port,
        const std::string &property) {
      proto::CheckProperty statement;
      statement.add_properties(property);
      auto p = statement.mutable_principal();
      auto auth = p->mutable_auth();
      auth->set_ip(ip);
      auth->set_port_lo(port);
      return quick_post<proto::Command::CHECK_PROPERTY>(statement);
    }

    int check_access(const std::string &ip, uint32_t port,
        const std::string &object) {
      proto::CheckAccess statement;
      statement.add_objects(object);
      auto p = statement.mutable_principal();
      auto auth = p->mutable_auth();
      auth->set_ip(ip);
      auth->set_port_lo(port);
      return quick_post<proto::Command::CHECK_ACCESS>(statement);
    }

    std::unique_ptr<proto::Attestation> check_attestation(
        const std::string &ip, uint32_t port) {
      proto::CheckAttestation statement;
      auto p = statement.mutable_principal();
      auto auth = p->mutable_auth();
      auth->set_ip(ip);
      auth->set_port_lo(port);
      auto resp = post(prepare<proto::Command::CHECK_ATTESTATION>(statement,
            myid_.c_str()));

      auto att = resp.extract_attestation();
      if (!att) {
        /// something might be wrong, try log it
        auto status = resp.status();
        log("%s, response = (%d, %s)",
            proto::statement_traits<proto::Command::CHECK_ATTESTATION>::name,
            status.first, status.second.c_str());
      }
      return att;
    }

    int check_worker_access(const std::string &ip, uint32_t port,
        const std::string &object) {
      proto::CheckAccess statement;
      statement.add_objects(object);
      auto p = statement.mutable_principal();
      auto auth = p->mutable_auth();
      auth->set_ip(ip);
      auth->set_port_lo(port);
      return quick_post<proto::Command::CHECK_WORKER_ACCESS>(statement);
    }
    ////

  private:

    template<proto::Command::Type type>
    inline int quick_post(
        const typename proto::statement_traits<type>::msg_type &statement) {
      return status_int<type>(post(prepare<type>(statement, myid_.c_str())));
    }

    template<proto::Command::Type type>
    inline std::unique_ptr<proto::Principal> quick_get_principal(
        const typename proto::statement_traits<type>::msg_type &statement) {
      auto cmd = prepare<type>(statement, myid_.c_str());
      auto resp = post(cmd);
      return resp.extract_principal();
    }

    template<proto::Command::Type type>
    int status_int(const proto::ResponseWrapper &r) {
      auto status = r.status();
      if (loglevel() == LOG_DEBUG) {
        log("%s, response = (%d, %s)", proto::statement_traits<type>::name,
            status.first, status.second.c_str());
        return status.first;
      } else {
        return r.status_int();
      }
    }


    proto::ResponseWrapper post(const proto::Command& cmd) {
      int ret = proto_send_msg(sock_, cmd);
      if (ret != 0) {
        std::stringstream err;
        err << "send failure, code " << ret;
        return proto::make_status_response(false, err.str());
      }
      auto result = proto::Response::default_instance().New();
      ret = proto_recv_msg(sock_, result);
      if (ret != 0) {
        std::stringstream err;
        err << "recv failure " << ret;
        return proto::make_status_response(false, err.str());
      }
      return proto::ResponseWrapper(result);
    }

    std::string myid_;
    std::string myip_;
    std::string daemon_path_;
    int sock_; 

};
}

static std::unique_ptr<latte::AttGuardClient> latte_client; 
static std::unique_ptr<SyscallProxy> syscall_gate;
static std::unordered_map<uint64_t, std::pair<uint32_t, uint32_t>> port_usage;
static std::string auth_speaker;
static std::string auth_ip;


// This must be called if a process intends to become an attester. It will
// clear stalled information (if any)
int liblatte_init(const char *myid, int run_as_iaas, const char *daemon_path) {
  syscall_gate = latte::utils::make_unique<SyscallProxyImpl>();
  std::string myip;
  auth_ip = latte::utils::get_myip();
  if (run_as_iaas) {
    myid = IAAS_IDENTITY;
    myip = "must_provide_ip_in_creation_as_iaas";
    auth_speaker = myid;
  } else {
    myip = auth_ip;
    int lo, hi;
    if (!myid || strcmp(myid, "") == 0) {
      if (syscall_gate->get_local_ports(&lo, &hi) < 0) {
        lo = 1;
        hi = 65535;
      }
      std::stringstream ss;
      ss << auth_ip << ":" << lo << "-" << hi;
      auth_speaker = ss.str();
    } else {
      auth_speaker = myid;
    }
  }
  if (auth_ip.compare("") == 0) {
    latte::log_err("failed to initialize liblatte principal identity");
    return -1;
  }
  if (!daemon_path || strcmp(daemon_path, "") == 0) {
    latte_client = latte::utils::make_unique<latte::AttGuardClient>(auth_speaker, myip);
  } else {
    latte_client = latte::utils::make_unique<latte::AttGuardClient>(auth_speaker, myip, daemon_path);
  }

  latte::log("liblatte core initialized for process %d, with id %s, ip: %s\n", getpid(),
      auth_speaker.c_str(), myip.c_str());
  return 0;
}

#define CHECK_LIB_INIT \
  if (!latte_client) { \
    latte::log_err("the library is not initialized");\
    return -1; \
  }
#define CHECK_LIB_INIT_PTR \
  if (!latte_client) { \
    latte::log_err("the library is not initialized");\
    return nullptr; \
  }

#define CHECK_NULL_PTR(ptr) \
  if (!ptr) { \
    latte::log("%s is invalid pointer", #ptr); \
    return EINVAL; \
  }

#define CHECK_NULL_PTR1(ptr, val) \
  if (!ptr) { \
    latte::log("%s is invalid pointer", #ptr); \
    return val; \
  }

static int _create_principal_new(uint64_t uuid, const char *image, const char *config,
    const char *ip, uint32_t port_lo, uint32_t port_hi) {
  latte::log("creating principal %llu, %s, %s, %d, %d, ip=%s\n", uuid, image, config,
      port_lo, port_hi, ip);
  if (!ip || *ip == '\0') {
    return latte_client->create_principal(uuid, image, config, 
        latte_client->myip(), port_lo, port_hi);
  } else {
    return latte_client->create_principal(uuid, image, config,
        ip, port_lo, port_hi);
  }
}

int liblatte_create_principal_new(uint64_t uuid, const char *image, const char *config,
    int nport, const char *new_ip) {
  // create principal, addresses are assgined by the library. Any statement
  // appended will be included must be "const char*" type, which means String
  // in Java. It will be included in misc config fields
  
  CHECK_LIB_INIT;
  /// allocate ports first (maybe assign new IP)
  int v = syscall_gate->alloc_child_ports(0, uuid, nport);
  if (v < 0) {
    latte::log_err("error in allocating child ports: %s\n", strerror(-v));
    return -1;
  }
  uint32_t port_lo = v;
  uint32_t port_hi = port_lo + nport;
  /// protobuf does not support null, make sure things are consistent
  if (!image) image = "";
  if (!config) config = "*";
  if (!_create_principal_new(uuid, image, config, new_ip, port_lo, port_hi)) {
    port_usage[uuid] = std::make_pair(port_lo, port_hi);
    return 0;
  }
  return 1;
}

int liblatte_create_principal(uint64_t uuid, const char *image, const char *config,
    int nport) {
  return ::liblatte_create_principal_new(uuid, image, config, nport, "");
}

int liblatte_create_principal_with_allocated_ports(uint64_t uuid, const char *image,
    const char *config, const char * ip, int port_lo, int port_hi) {
  if (!image) image = "";
  if (!config) config = "*";
  return ::_create_principal_new(uuid, image, config, ip, port_lo, port_hi);
}


/// Legacy API
int liblatte_create_image(const char *image_hash, const char *source_url,
    const char *source_rev, const char *misc_conf) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(image_hash);
  CHECK_NULL_PTR(source_url);
  latte::log("creating image %s, %s, %s, %s\n", image_hash, source_url,
      source_rev, misc_conf);
  //// the semantic will only endorse the source of an image
  if (!source_rev) source_rev = "";
  /// if no config specified, we treat all config acceptable.
  if (!misc_conf) misc_conf = "*";
  return latte_client->endorse_source(image_hash, misc_conf,
      latte::utils::format_image_source(source_url, source_rev));
}

int liblatte_post_object_acl(const char *obj_id, const char *requirement) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(obj_id);
  CHECK_NULL_PTR(requirement);
  latte::log("posting obj acl: %s, %s\n", obj_id, requirement);
  return latte_client->post_acl(obj_id, requirement);
}


/// legacy API
int liblatte_attest_principal_property(const char *ip, uint32_t port, const char *prop) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(prop);
  latte::log("attesting property: %s, %u, %s\n", ip, port, prop);
  return latte_client->check_property(ip, port, prop);
}

int liblatte_attest_principal_access(const char *ip, uint32_t port, const char *obj) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(obj);
  latte::log("attesting access: %s, %u, %s\n", ip, port, obj);
  return latte_client->check_access(ip, port, obj);
}

int liblatte_delete_principal_without_allocated_ports(uint64_t uuid) {
  latte::log("deleting principal: %llu\n", uuid);
  return latte_client->delete_principal(uuid);
}


int liblatte_delete_principal(uint64_t uuid) {
  return liblatte_delete_principal_without_allocated_ports(uuid);
}

void liblatte_set_log_level(int upto) {
  latte::setloglevel(upto);
}

/// helper

char* liblatte_get_principal(const char *ip, uint32_t lo, char **principal,
    size_t *size) {
  CHECK_LIB_INIT_PTR;
  CHECK_NULL_PTR1(ip, nullptr);
  CHECK_NULL_PTR1(principal, nullptr);
  CHECK_NULL_PTR1(size, nullptr);
  if (*principal) {
    latte::log("warning: config buffer not empty, possible memory leaking");
  }

  latte::log("get principal: %s:%u\n", ip, lo);
  std::unique_ptr<latte::proto::Principal> p = 
    latte_client->get_principal(ip, lo);
  if (p) {
    return wrap_proto_msg(*p, principal, size);
  } else {
    return nullptr;
  }

}

char* liblatte_get_local_principal(uint64_t uuid, char **principal,
    size_t *size) {
  CHECK_LIB_INIT_PTR;
  CHECK_NULL_PTR1(principal, nullptr);
  CHECK_NULL_PTR1(size, nullptr);
  if (*principal) {
    latte::log("warning: config buffer not empty, possible memory leaking");
  }

  latte::log("get local principal: %u\n", uuid);
  std::unique_ptr<latte::proto::Principal> p = 
    latte_client->get_local_principal(uuid);
  if (p) {
    return wrap_proto_msg(*p, principal, size);
  } else {
    return nullptr;
  }
}

int liblatte_get_metadata_config_easy(char *url, size_t *url_sz) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(url);
  CHECK_NULL_PTR(url_sz);
  if (*url_sz > URL_MAX_ALLOWED) {
    return -EINVAL;
  }
  latte::log("get metadata config");
  std::unique_ptr<latte::proto::MetadataConfig> config =
    latte_client->get_metadata_config();
  if (!config) return 0;
  *url_sz = snprintf(url, *url_sz, "%s:%u",
      config->ip().c_str(), config->port());
  return 1;
}

char* liblatte_get_metadata_config(char **metadata_config, size_t *size) {
  CHECK_LIB_INIT_PTR;
  CHECK_NULL_PTR1(metadata_config, nullptr);
  CHECK_NULL_PTR1(size, nullptr);
  if (*metadata_config) {
    latte::log("warning: config buffer not empty, possible memory leaking");
  }
  latte::log("get metadata config");
  std::unique_ptr<latte::proto::MetadataConfig> config =
    latte_client->get_metadata_config();
  if (!config) return nullptr;
  return wrap_proto_msg(*config, metadata_config, size);
}

int liblatte_endorse_principal(const char *ip, uint32_t port, uint64_t gn, int type,
    const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(property);
  latte::log("endorse principal: %s:%u:%llu, with %d:%s", ip, port, gn, type, property);
  return latte_client->endorse_principal(ip, port, gn, type, property);
}

int liblatte_revoke_principal(const char *, uint32_t , int , const char *) {
  latte::log_err("Not Implemented: revoke_principal");
  return -1;
}

int liblatte_endorse_image(const char *id, const char *config, const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(id);
  CHECK_NULL_PTR(property);
  if (!config) config = "*";
  latte::log("endorse image: %s:%s, with %s", id, config, property);
  return latte_client->endorse_image(id, config, latte::proto::Endorse::IMAGE, property);
}

int liblatte_endorse_source(const char *url, const char *rev, const char *config,
    const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(url);
  CHECK_NULL_PTR(rev);
  auto id = latte::utils::format_image_source(url, rev);
  CHECK_NULL_PTR(property);
  if (!config) config = "*";
  latte::log("endorse source: %s:%s, with %s", id.c_str(), config, property);
  return latte_client->endorse_source(id, config, latte::proto::Endorse::SOURCE, property);
}

int liblatte_revoke(const char *, const char *, int , const char *) {
  latte::log_err("Not Implemented: revoke");
  return -1;
}

int liblatte_endorse_membership(const char *ip, uint32_t port, uint64_t gn, const char *master) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(master);
  latte::log("endorse membership: %s:%u:%u, with %s", ip, port, gn, master);
  return latte_client->endorse_membership(ip, port, gn, master);
}

int liblatte_endorse_attester(const char *id, const char *config) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(id);
  if (!config) config = "*";
  latte::log("endorse attester: %s:%s", id, config);
  return latte_client->endorse_attester(id, config);
}

int liblatte_endorse_builder(const char *id, const char *config) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(id);
  if (!config) config = "*";
  latte::log("endorse builder: %s:%s", id, config);
  return latte_client->endorse_attester(id, config);
}

int liblatte_endorse_image_source(const char *id, const char * config,
    const char *source_url, const char *source_rev) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(id);
  CHECK_NULL_PTR(source_url);
  CHECK_NULL_PTR(source_rev);
  if (!config) config = "*";
  latte::log("endorse source: %s:%s with %s:%s",
      id, config, source_url, source_rev);
  return latte_client->endorse_source(id, config,
      latte::utils::format_image_source(source_url, source_rev));
}

int liblatte_check_property(const char *ip, uint32_t port, const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(property);
  latte::log("check property: if %s:%u having %s", ip, port, property);
  return latte_client->check_property(ip, port, property);
}

int liblatte_check_access(const char *ip, uint32_t port, const char *object) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(object);
  latte::log("check access: if %s:%u can access %s", ip, port, object);
  return latte_client->check_access(ip, port, object);
}

int liblatte_check_worker_access(const char *ip, uint32_t port, const char *object) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(object);
  latte::log("check worker access: if %s:%u can access %s", ip, port, object);
  return latte_client->check_worker_access(ip, port, object);
}

char* liblatte_check_attestation(const char *ip, uint32_t port, char **attestation,
    size_t *size) {
  CHECK_LIB_INIT_PTR;
  CHECK_NULL_PTR1(ip, nullptr);
  CHECK_NULL_PTR1(attestation, nullptr);
  CHECK_NULL_PTR1(size, nullptr);
  latte::log("check attestation: what %s:%u has", ip, port);
  std::unique_ptr<latte::proto::Attestation> att =
    latte_client->check_attestation(ip, port);

  if (!att) return nullptr;
  return latte::proto::wrap_proto_msg(*att, attestation, size);
}


int liblatte_speaker(char *speaker, int max_len) {
  size_t sz = max_len;
  if (sz > auth_speaker.size()) {
    sz = auth_speaker.size();
  }
  strncpy(speaker, auth_speaker.c_str(), sz);
  return 0;
}

int liblatte_authip(char *ip, int max_len) {
  size_t sz = max_len;
  if (sz > auth_ip.size()) {
    sz = auth_ip.size();
  }
  strncpy(ip, auth_ip.c_str(), sz);
  return 0;
}
