
#define BOOST_TEST_MODULE SerializationTest
#include <boost/test/unit_test.hpp>

#include "accessor_object.h"
#include "principal.h"
#include "image.h"
#include <iostream>


BOOST_AUTO_TEST_CASE(serialize_object) {
  latte::AccessorObject o("test1");
  o.add_acl("acl1");
  o.add_acl("acl2");
  o.add_acl("acl3");
  o.add_acl("acl1");
  auto jvalue = o.to_json();
  auto strval = jvalue.serialize();
  std::cerr << "ser result:" << strval << std::endl;

  auto v = web::json::value::parse(strval);
  std::unordered_set<std::string> expected_acls;
  expected_acls.insert("acl1");
  expected_acls.insert("acl2");
  expected_acls.insert("acl3");
  auto ary = v["acls"].as_array();
  BOOST_CHECK_EQUAL(ary.size(), 3);
  decltype(expected_acls) actual_acls;
  for (auto i = ary.cbegin(); i != ary.cend(); ++i) {
    actual_acls.insert(i->as_string());
  }
  BOOST_CHECK_EQUAL(v["name"].as_string(), "test1");
  BOOST_CHECK_EQUAL(actual_acls.size(), 3);
  for (auto i = actual_acls.begin(); i != actual_acls.end(); ++i) {
    BOOST_CHECK_EQUAL(expected_acls.count(*i), 1);
  }
}

BOOST_AUTO_TEST_CASE(deserialize_object) {
  std::string converted1 = "{ \"name\": \"test1\", \"acls\": []}";
  auto o1 = latte::AccessorObject::parse(converted1);
  BOOST_CHECK_EQUAL(o1.name(), "test1");
  BOOST_CHECK_EQUAL(o1.acls().size(), 0);

  std::string converted2 = "{ \"name\": \"test2\", \"acls\": [\"I am good\"]}";
  auto o2 = latte::AccessorObject::parse(converted2);
  BOOST_CHECK_EQUAL(o2.name(), "test2");
  BOOST_CHECK_EQUAL(o2.acls().size(), 1);

  std::string converted3 = "{ \"name\": \"test3\", \"acls\": [\"ha\", \"ha\"]}";
  auto o3 = latte::AccessorObject::parse(converted3);
  BOOST_CHECK_EQUAL(o3.name(), "test3");
  BOOST_CHECK_EQUAL(o3.acls().size(), 1);

  std::string converted_bad1 = "{ \"name\": \"test_bad1\", \"acls\": \"\"}";
  BOOST_CHECK_THROW(latte::AccessorObject::parse(converted_bad1), 
      web::json::json_exception);

  std::string converted_bad2 = "{ \"name\": \"test_bad2\", \"acls\": {}}";
  BOOST_CHECK_THROW(latte::AccessorObject::parse(converted_bad2), 
      web::json::json_exception);

  std::string converted_bad3 = "{ \"name\": \"test_bad3\"}";
  BOOST_CHECK_THROW(latte::AccessorObject::parse(converted_bad3), 
      web::json::json_exception);

  std::string converted_bad4 = "{ \"acls\": []}";
  BOOST_CHECK_THROW(latte::AccessorObject::parse(converted_bad4), 
      web::json::json_exception);
}

BOOST_AUTO_TEST_CASE(serialize_image) {
  latte::Image image("0xhash", "git://url", "0xrevision", "misc_config");
  auto v = image.to_json();
  auto strval = v.serialize();
  std::cerr << "ser result:" << strval << std::endl;

  auto dv = web::json::value::parse(strval);
  BOOST_CHECK_EQUAL(dv["hash"].as_string(), "0xhash");
  BOOST_CHECK_EQUAL(dv["url"].as_string(), "git://url");
  BOOST_CHECK_EQUAL(dv["revision"].as_string(), "0xrevision");
  BOOST_CHECK_EQUAL(dv["configs"].as_string(), "misc_config");

}

BOOST_AUTO_TEST_CASE(deserialize_image) {
  std::string s1 = "{ \"hash\": \"1\", \"url\": \"2\", \"revision\": \"3\", \
    \"configs\":\"4\"}";
  auto i1 = latte::Image::parse(s1);
  BOOST_CHECK_EQUAL(i1.hash(), "1");
  BOOST_CHECK_EQUAL(i1.url(), "2");
  BOOST_CHECK_EQUAL(i1.revision(), "3");
  BOOST_CHECK_EQUAL(i1.configs(), "4");

  std::string s_bad = "{ \"hash\": 1, \"url\": \"2\", \"revision\": \"3\", \
    \"configs\":\"4\"}";
  BOOST_CHECK_THROW(latte::Image::parse(s_bad), web::json::json_exception);

  std::string s_bad1 = "{\"url\": \"2\", \"revision\": \"3\", \
    \"configs\":\"4\"}";
  BOOST_CHECK_THROW(latte::Image::parse(s_bad1), web::json::json_exception);

  std::string s_bad2 = "{\"hash\":[], \"url\": \"2\", \"revision\": \"3\", \
    \"configs\":\"4\"}";
  BOOST_CHECK_THROW(latte::Image::parse(s_bad2), web::json::json_exception);
  std::string s_bad3 = "{\"hash\":{}, \"url\": \"2\", \"revision\": \"3\", \
    \"configs\":\"4\"}";
  BOOST_CHECK_THROW(latte::Image::parse(s_bad3), web::json::json_exception);
}

BOOST_AUTO_TEST_CASE(serialize_principal) {
  latte::Principal p(0x123, 100, 200, "image_hash", "config");
  auto v = p.to_json();
  auto sv = v.serialize();
  auto parsed = web::json::value::parse(sv);
  BOOST_CHECK_EQUAL(parsed["id"].as_string(), "0x123");
  BOOST_CHECK_EQUAL(parsed["lo"].as_string(), "100");
  BOOST_CHECK_EQUAL(parsed["hi"].as_string(), "200");
  BOOST_CHECK_EQUAL(parsed["image"].as_string(), "image_hash");
  BOOST_CHECK_EQUAL(parsed["configs"].as_string(), "config");
}

BOOST_AUTO_TEST_CASE(deserialize_principal) {
  std::string s1 = "{ \"id\": \"123\", \"lo\": \"100\", \"hi\": \"200\", \
    \"image\": \"image_hash\", \"configs\":\"misc\"}";
  auto v1 = latte::Principal::parse(s1);
  BOOST_CHECK_EQUAL(v1.id(), 123);

  std::string s2 = "{ \"id\": \"0x123\", \"lo\": \"100\", \"hi\": \"200\", \
    \"image\": \"image_hash\", \"configs\":\"misc\"}";
  auto v2 = latte::Principal::parse(s2);
  BOOST_CHECK_EQUAL(v2.id(), 0x123);

  std::string s_bad = "{ \"id\": \"bad\", \"lo\": \"100\", \"hi\": \"200\", \
    \"image\": \"image_hash\", \"configs\":\"misc\"}";
  BOOST_CHECK_THROW(latte::Principal::parse(s_bad), std::stringstream::failure);

  std::string s_bad1 = "{ \"id\": [], \"lo\": \"100\", \"hi\": \"200\", \
    \"image\": \"image_hash\", \"configs\":\"misc\"}";
  BOOST_CHECK_THROW(latte::Principal::parse(s_bad1), web::json::json_exception);

  std::string s_bad2 = "{ \"id\": {}, \"lo\": \"100\", \"hi\": \"200\", \
    \"image\": \"image_hash\", \"configs\":\"misc\"}";
  BOOST_CHECK_THROW(latte::Principal::parse(s_bad2), web::json::json_exception);
}
