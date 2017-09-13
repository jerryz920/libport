

#include "port_manager.h"
#include "principal.h"
#include "accessor_object.h"
#include "image.h"
#include "core_manager.h"
#include "metadata.h"
#include "mock.h"
#include "utils.h"

#include <chrono>
#include "cpprest/json.h"

#define BOOST_TEST_MODULE TestCore
#include <boost/test/unit_test.hpp>


namespace latte {


class MockMetadataClient: public MetadataServiceClient {
  public:
    MOCK_VOID_METHOD6(post_new_principal,
        const std::string&, principal_name,
        const std::string&, principal_ip, int, port_min,
        int, port_max, const std::string&, image_hash,
        const std::string&, configs, override)

    MOCK_VOID_METHOD4(post_new_image,
        const std::string&, image_hash,
        const std::string&, source_url,
        const std::string&, source_rev,
        const std::string&, misc_conf, override)

    MOCK_VOID_METHOD2(post_object_acl,
        const std::string&, obj_id,
        const std::string&, requirements, override)

    MOCK_VOID_METHOD2(endorse_image,
        const std::string&, image_hash,
        const std::string&, endorsement, override)

    MOCK_METHOD3(bool, has_property,
        const std::string&, principal_ip, int, port,
        const std::string&, property, override)

    MOCK_METHOD3(bool, can_access,
        const std::string&, principal_ip, int, port,
        const std::string&, access_object, override)
};

class MockSyscallProxy :public SyscallProxy {
  public:
  int64_t CALL_COUNT(get_local_ports) = 0;
  int RETURN_VALUE(get_local_ports) = 0; 
  typedef std::tuple< 
    std::remove_reference<int*>::type, 
    std::remove_reference<int*>::type> 
      ARG_TYPE(get_local_ports); 
  std::list<ARG_TYPE(get_local_ports)> CALL_ARGS(get_local_ports); 
  int get_local_ports(int* lo, int *hi) override {
    *lo = 1;
    *hi = 65535;
    CALL_COUNT(get_local_ports) += 1; 
    CALL_ARGS(get_local_ports).push_back(ARG_TYPE(get_local_ports)(lo, hi)); 
    printf("mock called\n");
    return RETURN_VALUE(get_local_ports); 
  }

  MOCK_METHOD3(int, set_child_ports, pid_t, pid, int, lo, int, hi, override)
  MOCK_METHOD2(int, add_reserved_ports, int, lo, int, hi, override)
  MOCK_METHOD2(int, del_reserved_ports, int, lo, int, hi, override)
  MOCK_METHOD0(int, clear_reserved_ports, override)
};


#include <stdio.h>
class MockCoreManager: public CoreManager {
  public:
  MockCoreManager(const std::string& metadata_url, const std::string& myip,
        const std::string& persistence_path, std::unique_ptr<SyscallProxy> s):
    CoreManager(metadata_url, myip, persistence_path, std::move(s)) {
      set_metadata_client(std::unique_ptr<MetadataServiceClient>(new MockMetadataClient));

    }
  void notify_created(std::string&& type, std::string&& key,
      web::json::value v) {
    CoreManager::notify_created(std::move(type), std::move(key), v);
  }

  void notify_deleted(std::string&& type, std::string&& key) {
    CoreManager::notify_deleted(std::move(type), std::move(key));
  }

  void notify_principal_created(const Principal& p) {
    notify_created(K_PRINCIPAL, utils::itoa<uint64_t, 
        utils::HexType>(p.id()), p.to_json());
  }

  void notify_principal_deleted(const Principal& p) {
    notify_deleted(K_PRINCIPAL, utils::itoa<uint64_t, 
        utils::HexType>(p.id()));
  }

  void notify_image_created(const Image& image) {
    notify_created(K_IMAGE, std::string(image.hash()), image.to_json());
  }

  void notify_image_deleted(const Image& image) {
    notify_deleted(K_IMAGE, std::string(image.hash()));
  }

  void notify_object_created(const AccessorObject& object) {
    notify_created(K_ACCESSOR, std::string(object.name()), object.to_json());
  }

  void notify_object_deleted(const AccessorObject& object) {
    notify_deleted(K_ACCESSOR, std::string(object.name()));
  }

  web::json::value obtain_root() {
    return CoreManager::get_config_root();
  }

  MockMetadataClient* get_mock_metadata_client() {
    return dynamic_cast<MockMetadataClient*>(CoreManager::get_metadata_client());
  }

  void set_new_config_root(web::json::value new_root) {
    CoreManager::set_new_config_root(new_root);
  }

  void sync() noexcept {
    CoreManager::sync();
  }

};




BOOST_AUTO_TEST_CASE(test_init) {
  CoreManager m ("http://localhost:10011", "192.168.0.1",
      "/tmp/libport-test-core-manager.json",
      std::unique_ptr<SyscallProxy>(new MockSyscallProxy));
}

BOOST_AUTO_TEST_CASE(test_send_sync) {
  MockCoreManager m ("http://localhost:10011", "192.168.0.1",
      "/tmp/libport-test-core-manager-send-sync.json",
      std::unique_ptr<SyscallProxy>(new MockSyscallProxy));
  m.start_sync_thread();

  auto p1 = latte::Principal(1, 100, 200, "image1", "config1");
  auto p2 = latte::Principal(2, 400, 500, "image2", "config2");
  auto i1 = latte::Image("hash1", "git://url1", "rev1", "config1");
  auto i2 = latte::Image("hash2", "git://url2", "rev2", "config2");
  auto o1 = latte::AccessorObject("o1");
  auto o2 = latte::AccessorObject("o2");

  // post two principals, one image, delete one principal,
  // post accessor, post one image, delete image, delete
  // accessor, post accessor, Check there are only 1, 1, 1
  // principal, image, and object each.

  m.notify_principal_created(p1);
  m.notify_principal_created(p2);
  m.notify_image_created(i1);
  m.notify_principal_deleted(p1);
  m.notify_object_created(o1);
  m.notify_image_created(i2);
  m.notify_image_deleted(i1);
  m.notify_object_deleted(o1);
  m.notify_object_created(o2);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  /// NOTE: We may get data race here if serialize operation changes internal
  // structure... But we won't need new things here to ensure a "test"
  // to be perfect correct.
  auto config_root = m.obtain_root();
  auto ps = config_root["principals"];
  auto is = config_root["images"];
  auto os = config_root["accessors"];
  BOOST_CHECK_EQUAL(ps.as_object().size(), 1);
  BOOST_CHECK_EQUAL(is.as_object().size(), 1);
  BOOST_CHECK_EQUAL(os.as_object().size(), 1);
  printf("principals: %s\n", ps.serialize().c_str());
  BOOST_CHECK_EQUAL(ps["0x2"]["lo"].as_string(), "400");
  BOOST_CHECK_EQUAL(ps["0x2"]["image"].as_string(), "image2");
  BOOST_CHECK_EQUAL(is["hash2"]["url"].as_string(), "git://url2");
  BOOST_CHECK_EQUAL(is["hash2"]["revision"].as_string(), "rev2");
  BOOST_CHECK_EQUAL(os["o2"]["name"].as_string(), "o2");
}

BOOST_AUTO_TEST_CASE(test_principal) {
  MockCoreManager m ("http://localhost:10011", "192.168.0.1",
      "/tmp/libport-test-core-manager-create-principal.json",
      std::unique_ptr<SyscallProxy>(new MockSyscallProxy));
  m.start_sync_thread();
  for (int i = 0; i < 5; i++) {
    std::stringstream ss, ss1;
    ss << "image" << i;
    ss1 << "config" << i;
    BOOST_CHECK_EQUAL(m.create_principal(i, ss.str(), ss1.str(), 10), 0);
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto config_root = m.obtain_root();
  auto ps = config_root["principals"];
  BOOST_CHECK_EQUAL(ps.as_object().size(), 5);
  for (int i = 0; i < 5; i++) {
    BOOST_CHECK(m.has_principal(i));
  }
  auto* mock_client = m.get_mock_metadata_client();
  BOOST_CHECK_EQUAL(mock_client->post_new_principal_call_count, 5);
  int i = 0;
  for (auto it = mock_client->post_new_principal_call_args.begin();
      it != mock_client->post_new_principal_call_args.end(); ++it) {
    std::stringstream ss, ss1;
    ss << "image" << i;
    ss1 << "config" << i;
    auto stri = latte::utils::itoa<int, latte::utils::HexType>(i);
    BOOST_CHECK_EQUAL(std::get<0>(*it), stri);
    BOOST_CHECK_EQUAL(std::get<1>(*it), "192.168.0.1");
    BOOST_CHECK_EQUAL(std::get<4>(*it), ss.str());
    BOOST_CHECK_EQUAL(std::get<5>(*it), ss1.str());
    i++;
  }

  m.delete_principal(1);
  m.delete_principal(2);
  m.delete_principal(0);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  BOOST_CHECK(!m.has_principal(0));
  BOOST_CHECK(!m.has_principal(1));
  BOOST_CHECK(!m.has_principal(2));
  for (int i = 3; i < 5; i++) {
    BOOST_CHECK(m.has_principal(i));
  }
  /// it's returned by value so we need to obtain it again
  config_root = m.obtain_root();
  ps = config_root["principals"];
  BOOST_CHECK_EQUAL(ps.as_object().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_image) {
  MockCoreManager m ("http://localhost:10011", "192.168.0.1",
      "/tmp/libport-test-core-manager-image.json",
      std::unique_ptr<SyscallProxy>(new MockSyscallProxy));
  m.start_sync_thread();
  for (int i = 0; i < 5; i++) {
    std::stringstream ss1, ss2, ss3, ss4;
    ss1 << "hash" << i;
    ss2 << "url" << i;
    ss3 << "rev" << i;
    ss4 << "config" << i;
    BOOST_CHECK_EQUAL(m.create_image(ss1.str(), ss2.str(), ss3.str(), ss4.str()), 0);
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto config_root = m.obtain_root();
  auto ps = config_root["images"];
  BOOST_CHECK_EQUAL(ps.as_object().size(), 5);
  for (int i = 0; i < 5; i++) {
    std::stringstream ss1;
    ss1 << "hash" << i;
    BOOST_CHECK(m.has_image(ss1.str()));
  }
  auto* mock_client = m.get_mock_metadata_client();
  BOOST_CHECK_EQUAL(mock_client->post_new_image_call_count, 5);
  int i = 0;
  for (auto it = mock_client->post_new_image_call_args.begin();
      it != mock_client->post_new_image_call_args.end(); ++it) {
    std::stringstream ss1, ss2, ss3, ss4;
    ss1 << "hash" << i;
    ss2 << "url" << i;
    ss3 << "rev" << i;
    ss4 << "config" << i;
    BOOST_CHECK_EQUAL(std::get<0>(*it), ss1.str());
    BOOST_CHECK_EQUAL(std::get<1>(*it), ss2.str());
    BOOST_CHECK_EQUAL(std::get<2>(*it), ss3.str());
    BOOST_CHECK_EQUAL(std::get<3>(*it), ss4.str());
    i++;
  }

}

BOOST_AUTO_TEST_CASE(test_object) {
  MockCoreManager m ("http://localhost:10011", "192.168.0.1",
      "/tmp/libport-test-core-manager-object.json",
      std::unique_ptr<SyscallProxy>(new MockSyscallProxy));
  m.start_sync_thread();
  for (int i = 0; i < 5; i++) {
    std::stringstream ss1;
    ss1 << "obj" << i;
    for (int j = 0; j < 5; j++) {
      BOOST_CHECK_EQUAL(m.post_object_acl(ss1.str(), "acl"), 0);
    }
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto config_root = m.obtain_root();
  auto ps = config_root["accessors"];
  BOOST_CHECK_EQUAL(ps.as_object().size(), 5);
  BOOST_CHECK_EQUAL(ps["obj1"]["acls"].as_array().size(), 1);
  for (int i = 0; i < 5; i++) {
    std::stringstream ss1;
    ss1 << "obj" << i;
    BOOST_CHECK(m.has_accessor(ss1.str()));
  }

  auto* mock_client = m.get_mock_metadata_client();
  BOOST_CHECK_EQUAL(mock_client->post_object_acl_call_count, 25);
  int i = 0;
  for (auto it = mock_client->post_object_acl_call_args.begin();
      it != mock_client->post_object_acl_call_args.end(); ++it) {
    std::stringstream ss1;
    ss1 << "obj" << i / 5;
    BOOST_CHECK_EQUAL(std::get<0>(*it), ss1.str());
    BOOST_CHECK_EQUAL(std::get<1>(*it), "acl");
    i++;
  }

}

BOOST_AUTO_TEST_CASE(test_recover) {
  std::string config_data = utils::read_file("../data/config-root.json");
  auto root = web::json::value::parse(config_data);
  auto mock_syscall_proxy = utils::make_unique<MockSyscallProxy>();
  const auto observer = mock_syscall_proxy.get();
  auto manager = utils::make_unique<MockCoreManager>("http://localhost:10011", 
      "192.168.0.1", "/tmp/libport-test-recover.json",
      std::move(mock_syscall_proxy));
  manager->set_new_config_root(root);
  manager->sync();

  BOOST_CHECK(manager->has_principal(3));
  BOOST_CHECK(manager->has_principal(4));
  BOOST_CHECK(manager->has_image("hash1"));
  BOOST_CHECK(manager->has_image("hash2"));
  BOOST_CHECK(manager->has_image("hash3"));
  BOOST_CHECK(manager->has_image("hash4"));
  BOOST_CHECK(manager->has_image("hash0"));
  BOOST_CHECK(manager->has_accessor("obj1"));
  BOOST_CHECK(manager->has_accessor("obj2"));
  BOOST_CHECK(manager->has_accessor("obj3"));
  BOOST_CHECK(manager->has_accessor("obj4"));
  BOOST_CHECK(manager->has_accessor("obj0"));
  BOOST_CHECK_EQUAL(observer->set_child_ports_call_count, 2);
  BOOST_CHECK_EQUAL(observer->add_reserved_ports_call_count, 2);
  int i = 0;
  for (auto it = observer->set_child_ports_call_args.cbegin();
      it != observer->set_child_ports_call_args.cend(); ++it, ++i) {

    if (std::get<0>(*it) == 3) {
      BOOST_CHECK_EQUAL(std::get<1>(*it), 65495);
      BOOST_CHECK_EQUAL(std::get<2>(*it), 65505);
    } else if (std::get<0>(*it) == 4) {
      BOOST_CHECK_EQUAL(std::get<1>(*it), 65485);
      BOOST_CHECK_EQUAL(std::get<2>(*it), 65495);
    } else {
      BOOST_FAIL("no such ID in test");
    }
  }
  for (auto it = observer->add_reserved_ports_call_args.cbegin();
      it != observer->add_reserved_ports_call_args.cend(); ++it, ++i) {

    if (std::get<0>(*it) == 65495) {
      BOOST_CHECK_EQUAL(std::get<1>(*it), 65505);
    } else if (std::get<0>(*it) == 65485) {
      BOOST_CHECK_EQUAL(std::get<1>(*it), 65495);
    } else {
      BOOST_FAIL("no such port in test");
    }
  }
}

}
