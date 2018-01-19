


#include "config.h"
#include "manager.h"
#include "comm.h"

#include "proto/statement.pb.h"
#include "proto/utils.h"
#include "test-include/mock-metadata-client.h"


#define BOOST_TEST_MODULE TestManager
#include <boost/test/unit_test.hpp>

using namespace latte;

BOOST_AUTO_TEST_CASE(test_init) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
}

void resp_ok(std::shared_ptr<latte::proto::Response> resp) {
  auto wrapper = proto::ResponseWrapper(std::move(resp));
  BOOST_CHECK_EQUAL(wrapper.status_int(), 1);
}

void resp_fail(std::shared_ptr<latte::proto::Response> resp) {
  auto wrapper = proto::ResponseWrapper(std::move(resp));
  BOOST_TEST_MESSAGE(wrapper.status().second);
  BOOST_CHECK_EQUAL(wrapper.status_int(), 0);
}

void resp_err(std::shared_ptr<latte::proto::Response> resp) {
  auto wrapper = proto::ResponseWrapper(std::move(resp));
  BOOST_CHECK_EQUAL(wrapper.status_int(), -1);
}

static inline latte::proto::Principal _P(int index) {
  latte::proto::Principal p;
  p.set_id(index);
  auto auth = p.mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(10000 + index * 100);
  auth->set_port_hi(10000 + index * 100 + 100);
  auto code = p.mutable_code();
  code->set_image("image");
  code->set_wildcard(true);
  auto& confmap = *code->mutable_config();
  confmap[latte::LEGACY_CONFIG_KEY] = "*";
  return p;
}


BOOST_AUTO_TEST_CASE(test_create_principal) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::Principal p = _P(0);

  auto cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  latte::proto::Principal p1 = _P(1);
  cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p1);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  latte::proto::Principal p2 = _P(0);
  cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p2);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);
  BOOST_CHECK_EQUAL(mclient->post_new_principal_call_count, 3);

  int index = 0;
  for (auto i = mclient->post_new_principal_call_args.begin();
      i != mclient->post_new_principal_call_args.end(); ++i) {
    if (index == 0) {
    BOOST_CHECK_EQUAL(std::get<0>(*i), "0:0"); // pid:gn
    BOOST_CHECK_EQUAL(std::get<2>(*i), 10000 + index * 100);
    BOOST_CHECK_EQUAL(std::get<3>(*i), 10000 + index * 100 + 100);
    } else if (index == 1) {
    BOOST_CHECK_EQUAL(std::get<0>(*i), "1:1"); // pid:gn
    BOOST_CHECK_EQUAL(std::get<2>(*i), 10000 + index * 100);
    BOOST_CHECK_EQUAL(std::get<3>(*i), 10000 + index * 100 + 100);
    } else {
    BOOST_CHECK_EQUAL(std::get<0>(*i), "0:2"); // pid:gn
    BOOST_CHECK_EQUAL(std::get<2>(*i), 10000 + 0 * 100);
    BOOST_CHECK_EQUAL(std::get<3>(*i), 10000 + 0 * 100 + 100);
    }
    BOOST_CHECK_EQUAL(std::get<1>(*i), "1.1.1.1");
    BOOST_CHECK_EQUAL(std::get<4>(*i), "image");
    BOOST_CHECK_EQUAL(std::get<5>(*i), "*");
    index++;
  }
}

BOOST_AUTO_TEST_CASE(test_create_principal_fail) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::CheckAccess invalid;

  auto cmd = latte::prepare<proto::Command::CHECK_ACCESS>(invalid);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);

  BOOST_CHECK_EQUAL(mclient->post_new_principal_call_count, 0);
}


BOOST_AUTO_TEST_CASE(test_remove_principal_no_gc) {

  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::Principal p = _P(0);

  auto cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  latte::proto::Principal p1 = _P(1);
  cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p1);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  latte::proto::Principal p2 = _P(2);
  cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p2);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p);
  BOOST_TEST_MESSAGE("remove first principal fail\n");
  cmd.set_uid(2);
  cmd.set_pid(101);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);


  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p);
  BOOST_TEST_MESSAGE("remove first principal\n");
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p1);
  BOOST_TEST_MESSAGE("remove second principal\n");
  cmd.set_uid(0);
  cmd.set_pid(101);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p2);
  BOOST_TEST_MESSAGE("remove third principal\n");
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p);
  BOOST_TEST_MESSAGE("remove duplicated principal\n");
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);
  BOOST_CHECK_EQUAL(mclient->remove_principal_call_count, 3);
}

BOOST_AUTO_TEST_CASE(test_remove_principal_gc) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();
  latte::proto::Principal p = _P(0);

  auto cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  latte::proto::Principal p1 = _P(1);
  cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p1);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  latte::proto::Principal p2 = _P(0);
  cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p2);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);
  latte::proto::Principal p3 = _P(0);
  cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p3);
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p);
  cmd.set_uid(1);
  cmd.set_pid(100);
  BOOST_TEST_MESSAGE("remove first principal\n");
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p1);
  cmd.set_uid(1);
  cmd.set_pid(100);
  BOOST_TEST_MESSAGE("remove second principal\n");
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p2);
  cmd.set_uid(1);
  cmd.set_pid(100);
  BOOST_TEST_MESSAGE("remove first principal\n");
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);
  cmd = latte::prepare<proto::Command::DELETE_PRINCIPAL>(p3);
  cmd.set_uid(1);
  cmd.set_pid(100);
  BOOST_TEST_MESSAGE("remove first principal\n");
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);
  BOOST_CHECK_EQUAL(mclient->remove_principal_call_count, 2);
}


BOOST_AUTO_TEST_CASE(test_post_object_acl) {
}

BOOST_AUTO_TEST_CASE(test_endorse_image) {

}

BOOST_AUTO_TEST_CASE(test_has_property) {
}

BOOST_AUTO_TEST_CASE(test_can_access) {
}

BOOST_AUTO_TEST_CASE(test_endorse_attester) {
}

BOOST_AUTO_TEST_CASE(test_endorse_membership) {
}

BOOST_AUTO_TEST_CASE(test_attest) {
}

BOOST_AUTO_TEST_CASE(test_can_worker_access) {
}
