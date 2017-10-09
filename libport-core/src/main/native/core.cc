
#include "libport.h"
#include "port-syscall.h"
#include "metadata.h"
#include "log.h"
#include "core_manager.h"
#include "image.h"
#include "principal.h"
#include "accessor_object.h"
#include "port_manager.h"
#include "utils.h"
#include "safe.h"
#include "pplx/threadpool.h"
#include <pthread.h>

#include <unordered_map>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>


using std::unordered_map;


/// Singleton instance of library storage

namespace {


std::string format_id(const std::string& ip, int lo, int hi) {
  std::stringstream ss;
  ss << ip << ":" << lo << "-" << hi;
  return ss.str();
}

std::string format_principal_id(uint64_t pid) {
  return latte::utils::itoa<uint64_t, latte::utils::HexType>(pid);
}

std::string format_system_time() {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  /// ctime string has exact 26 chars, 50 is enough.
  std::string result(50, ' ');
  ctime_r(&t, &result[0]);
  return result;
}

std::string check_proc(uint64_t pid) {
  std::stringstream exec_path_buf;
  exec_path_buf << "/proc/" << pid << "/exe";
  struct stat s;

  std::string exec_path = exec_path_buf.str();

  if (lstat(exec_path.c_str(), &s) < 0) {
    latte::log_err("diagnose pid %llu, lstat: %s", pid, strerror(errno));
    return "";
  }

  char *execbuf = (char*) malloc(s.st_size + 1);
  if (!execbuf) {
    latte::log_err("diagnose pid %llu, malloc: %s", pid, strerror(errno));
    return "";
  }
  int linksize = readlink(exec_path.c_str(), execbuf, s.st_size + 1);
  if (linksize < 0) {
    latte::log_err("diagnose pid %llu, readlink: %s", pid, strerror(errno));
    free(execbuf);
    return "";
  }
  if (linksize > s.st_size) {
    latte::log_err("diagnose pid %llu, link size changed between stat", pid);
    free(execbuf);
    return "";
  }
  execbuf[linksize] = '\0';
  auto result = std::string(execbuf, execbuf + linksize);
  free(execbuf);
  return result;
}


std::string make_default_restore_path(pid_t p) {
  std::stringstream ss;
  ss << "/var/run/libport/conf-" << p << ".json";
  return ss.str();
}

class SyscallProxyImpl: public latte::SyscallProxy {
  public:
    SyscallProxyImpl() {
      /// system usually reserves 32768-60000 ports to process as client.
      // Then if we update the port manager to 0-65535 then sometimes the set_local_port
      // does not work as expected. Instead, we read out the available local ports and
      // match it against get_child_ports
      int lo, hi;
      ::get_local_ports(&lo, &hi);

      int syslo, syshi;
      get_system_local_ports(&syslo, &syshi);

      if (lo < syslo || hi > syshi) {
        if (lo < syslo) lo = syslo;
        if (hi > syshi) hi = syshi;
        ::set_local_ports(lo, hi);
      }
    }

    int set_child_ports(pid_t pid, int lo, int hi) override {
      return ::set_child_ports(pid, lo, hi);
    }
    int get_local_ports(int* lo, int *hi) override {
      return ::get_local_ports(lo, hi);
    }
    int add_reserved_ports(int lo, int hi) override {
      return ::add_reserved_ports(lo, hi-1);
    }
    int del_reserved_ports(int lo, int hi) override {
      return ::del_reserved_ports(lo, hi-1);
    }
    int clear_reserved_ports() override {
      return ::clear_reserved_ports();
    }
    int alloc_child_ports(pid_t ppid, pid_t pid, int n) override {
      return ::alloc_child_ports(ppid, pid, n);
    }

    void get_system_local_ports(int *lo, int *hi) {
      std::ifstream f("/proc/sys/net/ipv4/ip_local_port_range");
      f >> *lo >> *hi;
    }
};

}


namespace latte {

  CoreManager::CoreManager(const std::string& metadata_url, const std::string& myip,
      const std::string& persistence_path, std::unique_ptr<SyscallProxy> proxy):
    terminate_(false),
    myip_(std::move(myip)),
    server_url_(metadata_url),
    persistence_path_(persistence_path),
    write_lock_(utils::make_unique<std::mutex>()),
    proxy_(std::move(proxy)) {

      if (proxy_->get_local_ports(&lo_, &hi_) != 0) {
        throw std::runtime_error("error fetching ip local ports");
      }
      //port_manager_ = utils::make_unique<PortManager>((uint32_t) lo_, (uint32_t) hi_);
      /// TODO:
      // Refactor this: metadata service client needs not be a concrete type, hard
      // for testing and replacing
      client_ = utils::make_unique<MetadataServiceClient>(
          metadata_url, format_id(myip_, lo_, hi_));
      config_root_[K_PRINCIPAL] = web::json::value::object();
      config_root_[K_IMAGE] = web::json::value::object();
      config_root_[K_ACCESSOR] = web::json::value::object();
  }

  CoreManager::~CoreManager() {
    /// FIXME: do we need any sort of memory barrier?
    if (!terminate_) {
      terminate();
    }
    if (this->sync_thread_) {
      try {
        this->sync_thread_->join();
      } catch(const std::system_error& e) {
        log_err("error quiting the sync thread: %s", e.what());
      }
    }
  }

  void CoreManager::reinitialize_metadata_client() {
    /// At this moment, parent needs to reset the metadata client only
      client_ = utils::make_unique<MetadataServiceClient>(
          server_url_, format_id(myip_, lo_, hi_));
  }

  std::unique_ptr<CoreManager> CoreManager::from_disk(const std::string& fpath,
      const std::string& server_url, const std::string &myip) {
    auto manager = utils::make_unique<CoreManager>(server_url, std::move(myip), fpath,
        utils::make_unique<SyscallProxyImpl>());
    std::string config_data = utils::read_file(fpath);
    if (config_data == "") {
      log_err("not able to read config data, assume clean manager");
      return manager;
    } else {
      try {
        manager->set_new_config_root(web::json::value::parse(config_data));
        manager->sync();
      } catch(const std::runtime_error& e) {
        log_err("caught error in recovering the core library, %s", e.what());
      }
    }
    return manager;
  }

  bool CoreManager::has_principal(uint64_t id) const {
    return principals_.find(id) != principals_.cend();
  }
  bool CoreManager::has_principal_by_port(uint32_t port) const {
    auto index = index_principals_.lower_bound(port);
    auto ptr = index->second;
    return ptr->lo() <= port && ptr->hi() > port;
  }
  bool CoreManager::has_image(std::string hash) const {
    return images_.find(hash) != images_.end();
  }
  bool CoreManager::has_accessor(std::string id) const {
    return accessors_.find(id) != accessors_.end();
  }

  void CoreManager::sync() noexcept {
    try {
      proxy_->clear_reserved_ports();
      sync_principals();
      sync_objects();
      sync_images();
    } catch(const std::runtime_error& e) {
      log_err("runtime error during syncing: %s", e.what());
    } catch (const web::json::json_exception& e) {
      log_err("json exception during syncing: %s", e.what());
    } catch (const web::http::http_exception& e) {
      log_err("http exception during syncing: %s", e.what());
    }

  }

  void CoreManager::sync_objects() {
    auto objs = config_root_[K_ACCESSOR].as_object();
    accessors_.clear();
    for (auto i = objs.begin(); i != objs.end(); ++i) {
      auto obj = AccessorObject::from_json(std::move(i->second));
      std::string name(obj.name());
      accessors_.emplace(name, utils::make_unique<AccessorObject>(std::move(obj)));
    }
  }

  void CoreManager::sync_images() {
    auto images = config_root_[K_IMAGE].as_object();
    images_.clear();
    for (auto i = images.begin(); i != images.end(); ++i) {
      auto image = Image::from_json(std::move(i->second));
      std::string name(image.hash());
      images_.emplace(name, utils::make_unique<Image>(std::move(image)));
    }
  }

  void CoreManager::sync_principals() {
    auto principals = config_root_[K_PRINCIPAL].as_object();
    principals_.clear();
    for (auto i = principals.begin(); i != principals.end(); ++i) {
      auto p = Principal::from_json(std::move(i->second));
      auto exec_path = check_proc(p.id());
      if (exec_path == "") {
        log("process %llu is not running! Syncing anyway\n", p.id());
      }
      principals_[p.id()] = new Principal(std::move(p));
      /// We should guarantee this method is not paralleled with other process running
      int err = proxy_->set_child_ports(p.id(), p.lo(), p.hi());
      if (err != 0) {
        log_err("can not recover the principal %llu with port %d-%d: %s\n",p.id(),
            p.lo(), p.hi(), strerror(-err));
      } else {
        proxy_->add_reserved_ports(p.lo(), p.hi());
      }
    }
  }

  int CoreManager::create_image(
      const std::string& image_hash,
      const std::string& source_url,
      const std::string& source_rev,
      const std::string& misc_conf
      ) noexcept {
    auto location = images_.emplace(image_hash,
        utils::make_unique<Image>(image_hash, source_url, source_rev, misc_conf));

    try {
      client_->post_new_image(image_hash, source_url, source_rev, misc_conf);
      notify_created(K_IMAGE, std::string(image_hash), 
          location.first->second->to_json());
      return 0;
    } catch (const std::runtime_error& e) {
      log_err("create_image, runtime error %s", e.what());
      images_.erase(image_hash);
      return -1;
    } catch (const web::json::json_exception& e) {
      log_err("create_image, json error %s", e.what());
      images_.erase(image_hash);
      return -2;
    } catch (const web::http::http_exception& e) {
      log_err("create_image, http error %s", e.what());
      images_.erase(image_hash);
      return -3;
    }
  }

  int CoreManager::create_principal(uint64_t id,
      const std::string& image_hash, const std::string& configs, int nport) noexcept {
    int v = proxy_->alloc_child_ports(0, id, nport);
    if (v < 0) {
      log_err("error in allocating child ports: %s\n", strerror(-v));
      return -1;
    }
    uint32_t port_low = v;
    uint32_t port_high = port_low + nport;

    try {
      std::string bearer = client_->post_new_principal(format_principal_id(id), 
          myip_, port_low, port_high, image_hash, configs);
      Principal* newp = new Principal(id,
            port_low, port_high, image_hash, configs, bearer);
      index_principals_[port_low] = newp;
      principals_[id] = newp;

      /// comment this off with alloc semantic
    log("before locking principal create");
        notify_created(K_PRINCIPAL, std::string(format_principal_id(id)),
            newp->to_json());
    } catch (const std::runtime_error& e) {
      log_err("create_principal, runtime error %s", e.what());
      //port_manager_->deallocate(port_low);
      /// del reserved port
      proxy_->del_reserved_ports(port_low, port_high);
      index_principals_.erase(port_low);
      principals_.erase(id);
      return -1;
    } catch (const web::json::json_exception& e) {
      log_err("create_principal, json error %s", e.what());
      /// del reserved port
      proxy_->del_reserved_ports(port_low, port_high);
      index_principals_.erase(port_low);
      principals_.erase(id);
      return -2;
    } catch (const web::http::http_exception& e) {
      log_err("create_principal, http error %s", e.what());
      /// del reserved port
      proxy_->del_reserved_ports(port_low, port_high);
      index_principals_.erase(port_low);
      principals_.erase(id);
      return -3;
    }
    return 0;
  }

  int CoreManager::post_object_acl(const std::string& obj_id,
      const std::string& requirement) noexcept {
    try {
      const auto insert_res = accessors_.emplace(obj_id,
          utils::make_unique<AccessorObject>(obj_id));
      const auto& obj_ptr = insert_res.first->second;
      obj_ptr->add_acl(requirement);
      client_->post_object_acl(obj_id, requirement);
      notify_created(K_ACCESSOR, std::string(obj_id),
          insert_res.first->second->to_json());
    } catch(const std::runtime_error& e) {
      log_err("create_object_acl, runtime error %s", e.what());
      accessors_.erase(obj_id);
      return -1;
    } catch (const web::json::json_exception& e) {
      log_err("create_object_acl, json error %s", e.what());
      accessors_.erase(obj_id);
      return -2;
    } catch (const web::http::http_exception& e) {
      log_err("create_object_acl, http error %s", e.what());
      accessors_.erase(obj_id);
      return -3;
    }
    return 0;
  }

  int CoreManager::endorse_image(const std::string& image_hash,
      const std::string& endorsement) noexcept {
    try {
      if (images_.find(image_hash) == images_.cend()) {
        log("Endorsing remote image: %s", image_hash.c_str());
      }
      client_->endorse_image(image_hash, endorsement);
    } catch(const std::runtime_error& e) {
      log_err("endorse_image, runtime error %s", e.what());
      return -1;
    } catch (const web::json::json_exception& e) {
      log_err("endorse_image, json error %s", e.what());
      return -2;
    } catch (const web::http::http_exception& e) {
      log_err("endorse_image, http error %s", e.what());
      return -3;
    }
    return 0;
  }

  bool CoreManager::attest_principal_property(const std::string& ip,
      uint32_t port, const std::string& property) noexcept {
    try {
      auto index = index_principals_.lower_bound(port);
    log("before locking attest property");
      auto ptr = index->second;
      if (ptr->lo() <= port && ptr->hi() > port) {
        return client_->has_property(ip, port, property, ptr->bearer());
      } else {
        return false;
      }
    } catch(const std::runtime_error& e) {
      log_err("attest_property, runtime error %s", e.what());
      return -1;
    } catch (const web::json::json_exception& e) {
      log_err("attest_property, json error %s", e.what());
      return -2;
    } catch (const web::http::http_exception& e) {
      log_err("attest_property, http error %s", e.what());
      return -3;
    }
  }

  bool CoreManager::attest_principal_access(const std::string& ip,
      uint32_t port, const std::string& obj) noexcept {
    try {
      auto index = index_principals_.lower_bound(port);
    log("before locking principal access");
      auto ptr = index->second;
      if (ptr->lo() <= port && ptr->hi() > port) {
        return client_->can_access(ip, port, obj, ptr->bearer());
      } else {
        return false;
      }
    } catch(const std::runtime_error& e) {
      log_err("attest_access, runtime error %s", e.what());
      return -1;
    } catch (const web::json::json_exception& e) {
      log_err("attest_access, json error %s", e.what());
      return -2;
    } catch (const web::http::http_exception& e) {
      log_err("attest_access, http error %s", e.what());
      return -3;
    }
  }

  int CoreManager::delete_principal(uint64_t uuid) noexcept {
    /// only delete principal and recycle the port at the moment
    auto record = principals_.find(uuid);
    if (record == principals_.cend()) {
      log("removing non-existed or non-process principal %llu\n", uuid);
      /// trying to diagnose the process pid, to see if it exist and there
      // is any unsync.
      auto exec_name = check_proc(uuid);
      if (exec_name != "") {
        log("diagnose: pid %llu exist, executing %s, but not in principal map",
            uuid, exec_name.c_str());
      }
      return -1;
    }
    client_->remove_principal(
        format_principal_id(record->second->id()),
        myip_, record->second->lo(), record->second->hi(), record->second->image(),
        record->second->configs());
    notify_deleted(K_PRINCIPAL, format_principal_id(uuid));
    fprintf(stderr, "debug: deleting reserved_ports\n");
    proxy_->del_reserved_ports(record->second->lo(), record->second->hi());
    index_principals_.erase(record->second->lo());
    principals_.erase(record);
    return 0;
  }

  void CoreManager::notify_created(std::string&& type, std::string&& key,
      web::json::value v) {
    std::unique_lock<std::mutex> guard(*this->write_lock_);
    this->sync_q_.emplace_back(std::move(type), std::move(key), true, v);
    this->sync_cond_.notify_one();
  }

  void CoreManager::notify_deleted(std::string&& type, std::string&& key) {
    std::unique_lock<std::mutex> guard(*this->write_lock_);
    this->sync_q_.emplace_back(std::move(type), std::move(key), false, 
        web::json::value::Null);
    this->sync_cond_.notify_one();
  }

  void CoreManager::start_sync_thread() {
    log("starting latte sync thread for process %lu\n", getpid());
    sync_thread_ = utils::make_unique<std::thread>([this]() noexcept {

      while (true) {
        decltype(this->sync_q_) pending;
        {
        /// We have to do this, because chrono::seconds require a reference, which
        // make it a ODR-use, which in addition requires definition of SYNC_DURATION.
        // We don't want the definition for real!
          constexpr int duration = SYNC_DURATION;
          std::unique_lock<std::mutex> guard(*this->write_lock_);
          this->sync_cond_.wait_for(guard, std::chrono::seconds(duration),
              [this] { return this->sync_q_.size() > 0 || this->terminate_; });
          // Quit the loop for terminate signal
          if (this->terminate_) {
            break;
          }
          pending.swap(this->sync_q_);
        }
        /// tuple[0] is the class type, principal, accessor, or images
        for (auto i = pending.begin(); i != pending.end(); i++) {
          auto& collection = this->config_root_[std::get<0>(*i)];
          if (std::get<2>(*i)) { // true means insertion
            printf("adding %s with %s\n", std::get<1>(*i).c_str(), std::get<3>(*i).serialize().c_str());
            collection[std::get<1>(*i)] = std::get<3>(*i);
          } else {
            printf("deleting %s\n", std::get<1>(*i).c_str());
            collection.erase(std::get<1>(*i));
          }
        }
        log("syncing config at %s", format_system_time().c_str());
        this->save(this->persistence_path_);
        pending.clear(); /// just for defensive if in case dctor makes some magic..
      }
    });
  }

  void CoreManager::save(const std::string &fpath) noexcept {
    try {
      std::ofstream out(fpath);
      out << this->config_root_.serialize();
    } catch(std::ios_base::failure e) {
      log_err("IO failure during save config: %s", e.what());
    } catch(std::runtime_error e) {
      log_err("something wrong happens during save config: %s", e.what());
    } catch(web::json::json_exception e) {
      log_err("json error during save config: %s", e.what());
    }
  }

  void CoreManager::stop() {
      log("%d, stop current mutex ptr: %p\n", getpid(), write_lock_.get());
      client_.reset(nullptr);
      if (sync_thread_) {
        terminate();
        try {
          this->sync_thread_->join();
        } catch(const std::system_error& e) {
          log_err("error quiting the sync thread: %s", e.what());
        }
      }
      /// NOTE: Only sync thread compete for this lock, and we must
      // ensure upper level only call libport from a single thread.
      this->write_lock_.reset(nullptr);
  }
  void CoreManager::reset_state(){
    // Now the thread is not running!
    // reinitialize the write lock!
    log("%d reseting mutex!", getpid());
    this->write_lock_ = utils::make_unique<std::mutex>();
    terminate_ = false;
  }

}


static std::unique_ptr<latte::CoreManager> core = nullptr;
static bool need_reinit_thread_pool = false;

static void prepare_to_fork() {
  latte::log("prepare to fork");
  if (core) {
    /// Thread pool must be cleared inside before fork.
    core->stop();
  }
  crossplat::threadpool::stop_shared();

}


static void parent_after_fork() {
  latte::log("after fork");
  crossplat::threadpool::reinit_shared();
  if (core) {
    core->reinitialize_metadata_client();
    core->reset_state();
    core->start_sync_thread();
  }
}

static void clear_stall_state() {
  if (core) {
    core.reset(nullptr);
  }
}

static void child_after_fork() {
  latte::log("child after fork");
  clear_stall_state();
  /// make sure the reserved ports are cleared after fork... Why we 
  //even copy it in kernel?
  ::clear_reserved_ports();
  need_reinit_thread_pool = true;
}

static pthread_once_t once;
static inline void register_fork_handle() {
  pthread_atfork(prepare_to_fork, parent_after_fork,
      child_after_fork);
}

// This must be called if a process intends to become an attester. It will
// clear stalled information (if any)
int libport_init(const char *server_url, const char *persistence_path, int run_as_iaas) {
  clear_stall_state();
  /// when to reinit??
  if (need_reinit_thread_pool) {
    crossplat::threadpool::reinit_shared();
  }
  // reinitialize the thread pool if needed
  pthread_once(&once, register_fork_handle);
  try {
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
    if (!persistence_path || strcmp(persistence_path, "") == 0) {
      core = latte::utils::make_unique<latte::CoreManager>(server_url, std::move(myip),
          make_default_restore_path(getpid()),
          latte::utils::make_unique<SyscallProxyImpl>());
    } else {
      core = latte::utils::make_unique<latte::CoreManager>(server_url, std::move(myip),
          persistence_path, latte::utils::make_unique<SyscallProxyImpl>());
    }
    core->start_sync_thread();
    latte::log("libport core initialized for process %d\n", getpid());
  } catch (const web::http::http_exception& e) {
    latte::log_err("init: http error %s", e.what());
    return -3;
  } catch (const std::system_error& e) {
    latte::log_err("init: system error %s", e.what());
    return -2;
  } catch (const std::runtime_error& e) {
    latte::log_err("init: runtime error %s", e.what());
    return -1;
  } catch (...) {
    latte::log_err("init: unknown exception caught");
  }
  return 0;
}

int libport_reinit(const char *server_url, const char *persistence_path, int run_as_iaas) {
  clear_stall_state();
  register_fork_handle();
  try {
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
    core = latte::CoreManager::from_disk(persistence_path, server_url, myip);
    core->start_sync_thread();
  } catch (const web::http::http_exception& e) {
    latte::log_err("reinit: http error %s", e.what());
    return -3;
  } catch (const std::system_error& e) {
    latte::log_err("reinit: system error %s", e.what());
    return -2;
  } catch (const std::runtime_error& e) {
    latte::log_err("reinit: runtime error %s", e.what());
    return -1;
  } catch (...) {
    latte::log_err("reinit: unknown exception caught");
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
  return core->endorse_image(image_hash, endorsement);
}

int attest_principal_property(const char *ip, uint32_t port, const char *prop) {
  CHECK_LIB_INIT_BOOL;
  latte::log("attesting property: %s, %u, %s\n", ip, port, prop);
  return core->attest_principal_property(ip, port, prop)? 1: 0;
}

int attest_principal_access(const char *ip, uint32_t port, const char *obj) {
  CHECK_LIB_INIT_BOOL;
  latte::log("attesting access: %s, %u, %s\n", ip, port, obj);
  return core->attest_principal_access(ip, port, obj)? 1: 0;
}

int delete_principal(uint64_t uuid) {
  CHECK_LIB_INIT_BOOL;
  latte::log("deleting principal: %llu\n", uuid);
  return core->delete_principal(uuid);
}


void libport_set_log_level(int upto) {
  latte::setloglevel(upto);
}



