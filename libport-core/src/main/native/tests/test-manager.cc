


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

void resp_attestation(std::shared_ptr<latte::proto::Response> resp) {
  auto wrapper = proto::ResponseWrapper(std::move(resp));
  BOOST_CHECK(wrapper.extract_attestation());
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
    BOOST_CHECK_EQUAL(std::get<1>(*i), "0:0"); // pid:gn
    BOOST_CHECK_EQUAL(std::get<3>(*i), 10000 + index * 100);
    BOOST_CHECK_EQUAL(std::get<4>(*i), 10000 + index * 100 + 100);
    } else if (index == 1) {
    BOOST_CHECK_EQUAL(std::get<1>(*i), "1:1"); // pid:gn
    BOOST_CHECK_EQUAL(std::get<3>(*i), 10000 + index * 100);
    BOOST_CHECK_EQUAL(std::get<4>(*i), 10000 + index * 100 + 100);
    } else {
    BOOST_CHECK_EQUAL(std::get<1>(*i), "0:2"); // pid:gn
    BOOST_CHECK_EQUAL(std::get<3>(*i), 10000 + 0 * 100);
    BOOST_CHECK_EQUAL(std::get<4>(*i), 10000 + 0 * 100 + 100);
    }
    BOOST_CHECK_EQUAL(std::get<2>(*i), "1.1.1.1");
    BOOST_CHECK_EQUAL(std::get<5>(*i), "image");
    BOOST_CHECK_EQUAL(std::get<6>(*i), "*");
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
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::PostACL failed_post;
  failed_post.set_name("obj");
  auto cmd = latte::prepare<proto::Command::POST_ACL>(std::move(failed_post));
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);

  latte::proto::PostACL post;
  post.set_name("obj");
  post.add_policies("property");

  cmd = latte::prepare<proto::Command::POST_ACL>(std::move(post));
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);
  BOOST_REQUIRE_EQUAL(mclient->post_object_acl_call_count, 1);
  auto &callarg = *mclient->post_object_acl_call_args.begin();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "obj");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), "property");
}

BOOST_AUTO_TEST_CASE(test_endorse_image) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::Endorse failed_endorse1;
  failed_endorse1.set_id("git://github.com");
  failed_endorse1.set_type(latte::proto::Endorse::SOURCE);
  auto ed1 = failed_endorse1.add_endorsements();
  ed1->set_type(latte::proto::Endorsement::OTHER);
  ed1->set_property("random property");
  auto cmd = latte::prepare<proto::Command::ENDORSE>(std::move(failed_endorse1));
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);

  latte::proto::Endorse failed_endorse2;
  failed_endorse2.set_id("image");
  failed_endorse2.set_type(latte::proto::Endorse::IMAGE);
  failed_endorse2.clear_endorsements();
  cmd = latte::prepare<proto::Command::ENDORSE>(std::move(failed_endorse2));
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);

  latte::proto::Endorse endorse;
  endorse.set_id("image");
  endorse.set_type(latte::proto::Endorse::IMAGE);
  auto &confmap = *endorse.mutable_config();
  confmap[latte::LEGACY_CONFIG_KEY] = "*";
  ed1 = endorse.add_endorsements();
  ed1->set_type(latte::proto::Endorsement::ATTESTER);
  ed1->set_property("attester");
  cmd = latte::prepare<proto::Command::ENDORSE>(std::move(endorse));
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);
  BOOST_REQUIRE_EQUAL(mclient->endorse_image_call_count, 1);

  auto &callarg = *mclient->endorse_image_call_args.begin();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "image");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), "attester");
  BOOST_CHECK_EQUAL(std::get<3>(callarg), "*");
}

BOOST_AUTO_TEST_CASE(test_has_property) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::CheckProperty failed_check;
  auto p = failed_check.mutable_principal();
  auto auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);

  auto cmd = latte::prepare<proto::Command::CHECK_PROPERTY>(std::move(failed_check));
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);

  latte::proto::CheckProperty check;
  p = check.mutable_principal();
  auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);
  check.add_properties("ppp");

  cmd = latte::prepare<proto::Command::CHECK_PROPERTY>(std::move(check));
  cmd.set_uid(1);
  cmd.set_pid(100);
  mclient->has_property_return_value = true;
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  BOOST_REQUIRE_EQUAL(mclient->has_property_call_count, 1);

  auto &callarg = *mclient->has_property_call_args.begin();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "1.1.1.1");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), 100);
  BOOST_CHECK_EQUAL(std::get<3>(callarg), "ppp");
}

BOOST_AUTO_TEST_CASE(test_can_access) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::CheckAccess failed_check;
  auto p = failed_check.mutable_principal();
  auto auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);

  auto cmd = latte::prepare<proto::Command::CHECK_ACCESS>(std::move(failed_check));
  cmd.set_uid(1);
  cmd.set_pid(100);
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);

  latte::proto::CheckAccess check;
  p = check.mutable_principal();
  auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);
  check.add_objects("ooo");

  cmd = latte::prepare<proto::Command::CHECK_ACCESS>(std::move(check));
  cmd.set_uid(1);
  cmd.set_pid(100);
  mclient->can_access_return_value = true;
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);

  BOOST_REQUIRE_EQUAL(mclient->can_access_call_count, 1);

  auto &callarg = *mclient->can_access_call_args.begin();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "1.1.1.1");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), 100);
  BOOST_CHECK_EQUAL(std::get<3>(callarg), "ooo");
}

static void _test_endorse_attester(std::shared_ptr<latte::LatteDispatcher> &manager,
    latte::proto::Endorse::Type t) {
  latte::proto::Endorse endorse;
  endorse.set_type(t);
  endorse.set_id("endorse_attester");
  auto &confmap = *endorse.mutable_config();
  confmap[latte::LEGACY_CONFIG_KEY] = "*";
    auto cmd = latte::prepare<latte::proto::Command::ENDORSE_ATTESTER_IMAGE>(
        std::move(endorse));
    manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);
}

BOOST_AUTO_TEST_CASE(test_endorse_attester) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  _test_endorse_attester(manager, latte::proto::Endorse::SOURCE);
  BOOST_REQUIRE_EQUAL(mclient->endorse_attester_on_source_call_count, 1);
  auto &callarg = mclient->endorse_attester_on_source_call_args.front();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "endorse_attester");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), "*");

  _test_endorse_attester(manager, latte::proto::Endorse::IMAGE);
  BOOST_REQUIRE_EQUAL(mclient->endorse_attester_call_count, 1);
  auto &callarg1 = mclient->endorse_attester_call_args.front();
  BOOST_CHECK_EQUAL(std::get<1>(callarg1), "endorse_attester");
  BOOST_CHECK_EQUAL(std::get<2>(callarg1), "*");

}

static void _test_endorse_membership(std::shared_ptr<latte::LatteDispatcher> &manager,
    bool fail) {
  latte::proto::EndorsePrincipal endorse;
  auto p = endorse.mutable_principal();
  p->set_gn(10010);
  auto auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);
  auto code = p->mutable_code();
  auto &confmap = *code->mutable_config();
  confmap[latte::LEGACY_CONFIG_KEY] = "*";

  if (!fail) {
    auto e = endorse.add_endorsements();
    e->set_type(latte::proto::Endorsement::MEMBERSHIP);
    e->set_property("membership");
    auto cmd = latte::prepare<latte::proto::Command::ENDORSE_MEMBERSHIP>(
        std::move(endorse));
    manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
        resp_ok);
  } else {
    auto cmd = latte::prepare<latte::proto::Command::ENDORSE_MEMBERSHIP>(
        std::move(endorse));
    manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
        resp_fail);
  }
}

BOOST_AUTO_TEST_CASE(test_endorse_membership) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();
  _test_endorse_membership(manager, true);
  BOOST_REQUIRE_EQUAL(mclient->endorse_membership_call_count, 0);
  _test_endorse_membership(manager, false);
  BOOST_REQUIRE_EQUAL(mclient->endorse_membership_call_count, 1);
  auto &callarg = mclient->endorse_membership_call_args.front();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "1.1.1.1");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), 100);
  BOOST_CHECK_EQUAL(std::get<3>(callarg), 10010);
  BOOST_CHECK_EQUAL(std::get<4>(callarg), "membership");
  BOOST_CHECK_EQUAL(std::get<5>(callarg), "*");
}

BOOST_AUTO_TEST_CASE(test_attest) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();
  latte::proto::CheckAttestation check;
  auto p = check.mutable_principal();
  auto auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);
  auto cmd = latte::prepare<latte::proto::Command::CHECK_ATTESTATION>(
      std::move(check));
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_attestation);
  BOOST_REQUIRE_EQUAL(mclient->attest_call_count, 1);
  auto &callarg = mclient->attest_call_args.front();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "1.1.1.1");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), 100);
}

BOOST_AUTO_TEST_CASE(test_can_worker_access) {
  latte::MockMetadataClient *mclient = new latte::MockMetadataClient();
  latte::init_manager(mclient);
  auto manager = latte::get_manager();

  latte::proto::CheckAccess check_fail;
  auto p = check_fail.mutable_principal();
  auto auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);
  auto cmd = latte::prepare<latte::proto::Command::CHECK_WORKER_ACCESS>(
      std::move(check_fail));
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_fail);
  BOOST_REQUIRE_EQUAL(mclient->can_worker_access_call_count, 0);

  mclient->can_worker_access_return_value = true;

  latte::proto::CheckAccess check;
  p = check.mutable_principal();
  auth = p->mutable_auth();
  auth->set_ip("1.1.1.1");
  auth->set_port_lo(100);
  check.add_objects("obj");
  cmd = latte::prepare<latte::proto::Command::CHECK_WORKER_ACCESS>(
      std::move(check));
  manager->dispatch(std::make_shared<proto::Command>(std::move(cmd)),
      resp_ok);
  BOOST_REQUIRE_EQUAL(mclient->can_worker_access_call_count, 1);
  auto &callarg = mclient->can_worker_access_call_args.front();
  BOOST_CHECK_EQUAL(std::get<1>(callarg), "1.1.1.1");
  BOOST_CHECK_EQUAL(std::get<2>(callarg), 100);
  BOOST_CHECK_EQUAL(std::get<3>(callarg), "obj");
}
