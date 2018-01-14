


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
  latte::MockMetadataClient mclient;
  latte::init_manager(&mclient);
}

void resp_ok(std::shared_ptr<latte::proto::Response> resp) {
  auto wrapper = proto::ResponseWrapper(std::move(resp));
  BOOST_CHECK_EQUAL(wrapper.status_int(), 1);
}

void resp_fail(std::shared_ptr<latte::proto::Response> resp) {
  auto wrapper = proto::ResponseWrapper(std::move(resp));
  BOOST_CHECK_EQUAL(wrapper.status_int(), 0);
}

void resp_err(std::shared_ptr<latte::proto::Response> resp) {
  auto wrapper = proto::ResponseWrapper(std::move(resp));
  BOOST_CHECK_EQUAL(wrapper.status_int(), -1);
}


BOOST_AUTO_TEST_CASE(test_create_principal) {
  latte::MockMetadataClient mclient;
  latte::init_manager(&mclient);
  auto manager = latte::get_manager();

  latte::proto::Principal p;
  p.set_id(1);
  auto auth = p.mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_hi(100);
  auth->set_port_hi(200);
  auto code = p.mutable_code();
  code->set_image("image");
  code->set_wildcard(true);
  auto& confmap = *code->mutable_config();
  confmap[latte::LEGACY_CONFIG_KEY] = "*";

  auto cmd = latte::prepare<proto::Command::CREATE_PRINCIPAL>(p);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  BOOST_CHECK_EQUAL(mclient.post_new_principal_call_count, 1);
  for (auto i = mclient.post_new_principal_call_args.begin();
      i != mclient.post_new_principal_call_args.end(); ++i) {
    BOOST_CHECK_EQUAL(std::get<0>(*i), "1:0"); // pid:gn
    BOOST_CHECK_EQUAL(std::get<1>(*i), "1.1.1.1");
    BOOST_CHECK_EQUAL(std::get<2>(*i), 100);
    BOOST_CHECK_EQUAL(std::get<3>(*i), 200);
    BOOST_CHECK_EQUAL(std::get<4>(*i), "image");
    BOOST_CHECK_EQUAL(std::get<5>(*i), "*");
  }
}

BOOST_AUTO_TEST_CASE(test_create_principal_fail) {
  latte::MockMetadataClient mclient;
  latte::init_manager(&mclient);
  auto manager = latte::get_manager();

  latte::proto::CheckAccess invalid;

  auto cmd = latte::prepare<proto::Command::CHECK_ACCESS>(invalid);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);

  BOOST_CHECK_EQUAL(mclient.post_new_principal_call_count, 0);
}


BOOST_AUTO_TEST_CASE(test_remove_principal_no_gc) {
}

BOOST_AUTO_TEST_CASE(test_remove_principal_gc) {
}

BOOST_AUTO_TEST_CASE(test_remove_principal_matching) {
}

BOOST_AUTO_TEST_CASE(test_remove_principal_no_matching) {
}

BOOST_AUTO_TEST_CASE(test_post_new_image) {
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
