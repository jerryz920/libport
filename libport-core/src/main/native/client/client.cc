


#include "port-syscall.h"
#include "libport.h"
#include "log.h"
#include "utils.h"
#include "safe.h"
#include "config.h"
#include "../proto/statement.pb.h"
#include "../proto/traits.h"
#include "../proto/utils.h"
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

    AttGuardClient(std::string myip): AttGuardClient(myip, DEFAULT_DAEMON_PATH) { }

    AttGuardClient(std::string myip, std::string daemon_path):
        myip_(std::move(myip)), daemon_path_(std::move(daemon_path)), sock_(0) {
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
      auth->set_port_hi(lo);
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

    std::unique_ptr<proto::Principal> get_local_principal(uint64_t uuid, uint64_t gn) {
      proto::Principal lookup;
      lookup.set_id(uuid);
      lookup.set_gn(gn);
      return quick_get_principal<proto::Command::GET_PRINCIPAL>(lookup);
    }

    std::unique_ptr<proto::MetadataConfig> get_metadata_config() {
      proto::Empty placeholder;
      auto resp = post(prepare<proto::Command::GET_METADATA_CONFIG>(placeholder));
      return resp.extract_metadata_config();
    }

    int endorse_principal(const std::string &ip, uint32_t port, int type,
        const std::string &property) {
      proto::EndorsePrincipal statement;
      auto p = statement.mutable_principal();
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

    int endorse(const std::string &id, const std::string &config,
        int type, const std::string &property) {
      proto::Endorse statement;
      statement.set_id(id);
      auto endorsement = statement.add_endorsements();
      set_type(endorsement, type);
      endorsement->set_property(property);
      auto& confmap = *statement.mutable_config();
      confmap[LEGACY_CONFIG_KEY] = config;
      return quick_post<proto::Command::ENDORSE>(statement);
    }

    int revoke(const std::string &, const std::string &, 
        int , const std::string &) {
      log_err("Not Implemented: revoke");
      return -1;
    }

    int endorse_membership(const std::string &ip, uint32_t port,
        const std::string &property) {
      proto::EndorsePrincipal statement;
      auto p = statement.mutable_principal();
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
      auto resp = post(prepare<proto::Command::CHECK_ATTESTATION>(statement));
      return resp.extract_attetation();
    }
    ////

  private:

    template<proto::Command::Type type>
    inline int quick_post(
        const typename proto::statement_traits<type>::msg_type &statement) {
      return status_int<type>(post(prepare<type>(statement)));
    }

    template<proto::Command::Type type>
    inline std::unique_ptr<proto::Principal> quick_get_principal(
        const typename proto::statement_traits<type>::msg_type &statement) {
      auto cmd = prepare<type>(statement);
      auto resp = post(cmd);
      return resp.extract_principal();
    }

    template<proto::Command::Type type>
    proto::Command prepare(
        const typename proto::statement_traits<type>::msg_type &statement) {
      proto::Command cmd;
      cmd.set_id(utils::gen_rand_uint64());
      cmd.set_type(type);
      auto stmt_field = cmd.mutable_statement();
      stmt_field->PackFrom(statement);
      return cmd;
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
        return proto::ResponseWrapper::make_status_response(false, err.str());
      }
      auto result = proto::Response::default_instance().New();
      ret = proto_recv_msg(sock_, result);
      if (ret != 0) {
        std::stringstream err;
        err << "recv failure" << ret;
        return proto::ResponseWrapper::make_status_response(false, err.str());
      }
      return proto::ResponseWrapper(result);
    }

    std::string myip_;
    std::string daemon_path_;
    int sock_; 

};
}

static std::unique_ptr<latte::AttGuardClient> latte_client; 
static std::unique_ptr<SyscallProxy> syscall_gate;




// This must be called if a process intends to become an attester. It will
// clear stalled information (if any)
int libport_init(int run_as_iaas, const char *daemon_path) {
  std::string myip;
  if (run_as_iaas) {
    myip = IAAS_IDENTITY;
  } else {
    myip = latte::utils::get_myip();
  }
  if (myip.compare("") == 0) {
    latte::log_err("failed to initialize libport principal identity");
    return -1;
  }
  latte_client = latte::utils::make_unique<latte::AttGuardClient>(myip, daemon_path);
  syscall_gate = latte::utils::make_unique<SyscallProxyImpl>();

  latte::log("libport core initialized for process %d\n", getpid());
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



int _create_principal_new(uint64_t uuid, const char *image, const char *config,
    const char *ip, uint32_t port_lo, uint32_t port_hi) {
  if (!ip || *ip == '\0') {
    return latte_client->create_principal(uuid, image, config, 
        latte_client->myip(), port_lo, port_hi);
  } else {
    return latte_client->create_principal(uuid, image, config,
        ip, port_lo, port_hi);
  }
}

int create_principal_new(uint64_t uuid, const char *image, const char *config,
    int nport, const char *new_ip) {
  // create principal, addresses are assgined by the library. Any statement
  // appended will be included must be "const char*" type, which means String
  // in Java. It will be included in misc config fields
  
  CHECK_LIB_INIT;
  latte::log("creating principal %llu, %s, %s, %d, %s\n", uuid, image, config, nport, new_ip);
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
  if (!config) config = "";
  return _create_principal_new(uuid, image, config, new_ip, port_lo, port_hi);
}

int create_principal(uint64_t uuid, const char *image, const char *config,
    int nport) {
  return ::create_principal_new(uuid, image, config, nport, "");
}

/// Legacy API
int create_image(const char *image_hash, const char *source_url,
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

int post_object_acl(const char *obj_id, const char *requirement) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(obj_id);
  CHECK_NULL_PTR(requirement);
  latte::log("posting obj acl: %s, %s\n", obj_id, requirement);
  return latte_client->post_acl(obj_id, requirement);
}

int endorse_image_new(const char *image_hash, const char *config,
    const char *endorsement) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(image_hash);
  CHECK_NULL_PTR(config);
  CHECK_NULL_PTR(endorsement);
  latte::log("endorsing image: %s, %s, %s\n", image_hash, endorsement, config);
  return latte_client->endorse(image_hash, config, 
      latte::proto::Endorsement::OTHER, endorsement);
}

/// Should delete this API
int endorse_image(const char *image_hash, const char *endorsement) {
  return endorse_image_new(image_hash, "", endorsement);
}

/// legacy API
int attest_principal_property(const char *ip, uint32_t port, const char *prop) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(prop);
  latte::log("attesting property: %s, %u, %s\n", ip, port, prop);
  return latte_client->check_property(ip, port, prop);
}

int attest_principal_access(const char *ip, uint32_t port, const char *obj) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(obj);
  latte::log("attesting access: %s, %u, %s\n", ip, port, obj);
  return latte_client->check_access(ip, port, obj);
}

int delete_principal(uint64_t uuid) {
  CHECK_LIB_INIT;
  latte::log("deleting principal: %llu\n", uuid);
  return latte_client->delete_principal(uuid);
}

void libport_set_log_level(int upto) {
  latte::setloglevel(upto);
}

/// helper

char* get_principal(const char *ip, uint32_t lo, char **principal,
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

char* get_local_principal(uint64_t uuid, uint64_t gn, char **principal,
    size_t *size) {
  CHECK_LIB_INIT_PTR;
  CHECK_NULL_PTR1(principal, nullptr);
  CHECK_NULL_PTR1(size, nullptr);
  if (*principal) {
    latte::log("warning: config buffer not empty, possible memory leaking");
  }

  latte::log("get local principal: %u:%u\n", uuid, gn);
  std::unique_ptr<latte::proto::Principal> p = 
    latte_client->get_local_principal(uuid, gn);
  if (p) {
    return wrap_proto_msg(*p, principal, size);
  } else {
    return nullptr;
  }
}

int get_metadata_config_easy(char *url, size_t *url_sz) {
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

char* get_metadata_config(char **metadata_config, size_t *size) {
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

int endorse_principal(const char *ip, uint32_t port, int type,
    const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(property);
  latte::log("endorse principal: %s:%u, with %d:%s", ip, port, type, property);
  return latte_client->endorse_principal(ip, port, type, property);
}

int revoke_principal(const char *, uint32_t , int , const char *) {
  latte::log_err("Not Implemented: revoke_principal");
  return -1;
}

int endorse_image_latte(const char *id, const char *config, int type, const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(id);
  CHECK_NULL_PTR(property);
  if (!config) config = "*";
  latte::log("endorse image: %s:%s, with %d:%s", id, config, type, property);
  return latte_client->endorse(id, config, type, property);
}

int endorse_source(const char *url, const char *rev,
    const char *config, int type, const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(url);
  CHECK_NULL_PTR(rev);
  CHECK_NULL_PTR(property);
  if (!config) config = "*";
  std::string id = latte::utils::format_image_source(url, rev);
  latte::log("endorse source: %s:%s, with %d:%s",
      id.c_str(), config, type, property);
  return latte_client->endorse(id, config, type, property);
}

int revoke(const char *, const char *, int , const char *) {
  latte::log_err("Not Implemented: revoke");
  return -1;
}

int endorse_membership(const char *ip, uint32_t port, const char *master) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(master);
  latte::log("endorse membership: %s:%u, with %s", ip, port, master);
  return latte_client->endorse_membership(ip, port, master);
}

int endorse_attester(const char *id, const char *config) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(id);
  if (!config) config = "*";
  latte::log("endorse attester: %s:%s", id, config);
  return latte_client->endorse_attester(id, config);
}

int endorse_builder(const char *id, const char *config) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(id);
  if (!config) config = "*";
  latte::log("endorse builder: %s:%s", id, config);
  return latte_client->endorse_attester(id, config);
}

int endorse_source(const char *id, const char *config,
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

int check_property(const char *ip, uint32_t port, const char *property) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(property);
  latte::log("check property: if %s:%u having %s", ip, port, property);
  return latte_client->check_property(ip, port, property);
}

int check_access(const char *ip, uint32_t port, const char *object) {
  CHECK_LIB_INIT;
  CHECK_NULL_PTR(ip);
  CHECK_NULL_PTR(object);
  latte::log("check access: if %s:%u can access %s", ip, port, object);
  return latte_client->check_access(ip, port, object);
}

char* check_attestation(const char *ip, uint32_t port, char **attestation,
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
