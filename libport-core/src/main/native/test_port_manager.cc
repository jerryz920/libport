//#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE PortManagerTest
#include <boost/test/unit_test.hpp>

#include "include/port_manager.h"
#include <stdio.h>

BOOST_AUTO_TEST_CASE(manager_init) {

  latte::PortManager m;
  BOOST_CHECK_EQUAL(m.low(), 1);
  BOOST_CHECK_EQUAL(m.high(), 65535);
  latte::PortManager m1(5,1000);
  BOOST_CHECK_EQUAL(m1.low(), 5);
  /// Should be [5, 1000)
  BOOST_CHECK_EQUAL(m1.high(), 1000);


}

BOOST_AUTO_TEST_CASE(manager_allocate) {

  latte::PortManager m(1, 2001);
  auto p1 = m.allocate(1000);
  auto p2 = m.allocate(1000);
  BOOST_CHECK_EQUAL(p1.second - p1.first, 1000);
  BOOST_CHECK_EQUAL(p2.second - p2.first, 1000);
  BOOST_CHECK_THROW(m.allocate(100), std::runtime_error);

}
BOOST_AUTO_TEST_CASE(manager_deallocate) {
  latte::PortManager m(1, 2001);
  auto p1 = m.allocate(1000);
  auto p2 = m.allocate(1000);
  latte::PortManager::PortPair p3;
  BOOST_CHECK_THROW(m.deallocate(p1.first + 1), std::runtime_error);
  BOOST_CHECK_THROW(m.deallocate(p1.second + 1), std::runtime_error);
  BOOST_CHECK_THROW(m.allocate(100), std::runtime_error);
  BOOST_CHECK_NO_THROW(m.deallocate(p1.first));
  BOOST_CHECK_NO_THROW((p3=m.allocate(100)));
  BOOST_CHECK_EQUAL(p3.second - p3.first, 100);
  BOOST_CHECK_NO_THROW(m.deallocate(p2.first));
  for (auto i = 0; i < 19; i++) {
    BOOST_CHECK_NO_THROW(m.allocate(100));
  }
  BOOST_CHECK_THROW(m.allocate(100), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(manager_check_allocated) {
  latte::PortManager m(1, 2001);
  auto p = m.allocate(200);
  auto p2 = m.allocate(200);
  auto p1 = m.allocate(20);
  m.deallocate(p.first);
  m.deallocate(p2.first);
  for (auto i = p1.first; i < p1.second; i++) {
    BOOST_CHECK(m.is_allocated(i));
    for (auto j = i + 1; j <= p1.second + 10; j++) {
      BOOST_CHECK(m.is_allocated(i, j));
    }
    for (auto j = 1; j <= 10; j++) {
      BOOST_CHECK(m.is_allocated(i - j, p1.second));
    }
  }
}

BOOST_AUTO_TEST_CASE(manager_allocate_exact) {
  printf("begin of allcoate exact\n");
  latte::PortManager m(1, 2001);
  auto p = m.allocate(1, 10);
  BOOST_CHECK_EQUAL(p.first, 1);
  BOOST_CHECK_EQUAL(p.second, 10);
  auto p1 = m.allocate(10, 20);
  BOOST_CHECK_EQUAL(p1.first, 10);
  BOOST_CHECK_EQUAL(p1.second, 20);
  auto p2 = m.allocate(20, 30);
  BOOST_CHECK_EQUAL(p2.first, 20);
  BOOST_CHECK_EQUAL(p2.second, 30);
  auto p3 = m.allocate(30, 31);
  BOOST_CHECK_EQUAL(p3.first, 30);
  BOOST_CHECK_EQUAL(p3.second, 31);
  auto p4 = m.allocate(31, 32);
  BOOST_CHECK_EQUAL(p4.first, 31);
  BOOST_CHECK_EQUAL(p4.second, 32);
  BOOST_CHECK_THROW(m.allocate(10, 11), std::runtime_error);
  BOOST_CHECK_THROW(m.allocate(31, 35), std::runtime_error);
  BOOST_CHECK(m.is_allocated(1));
  BOOST_CHECK(m.is_allocated(1,5));
  BOOST_CHECK(m.is_allocated(1, 20));
  BOOST_CHECK(m.is_allocated(10,15));
  BOOST_CHECK(m.is_allocated(10,35));
  BOOST_CHECK(m.is_allocated(0,35));

  latte::PortManager m1(1, 2001);
  m1.allocate(10, 20);
  m1.allocate(30, 40);
  BOOST_CHECK(!m1.is_allocated(20, 30));
  BOOST_CHECK(m1.is_allocated(20, 31));
  BOOST_CHECK(m1.is_allocated(19, 30));
  BOOST_CHECK(m1.is_allocated(20, 41));
  BOOST_CHECK(m1.is_allocated(1, 15));
  BOOST_CHECK(m1.is_allocated(34, 41));
  BOOST_CHECK(m1.is_allocated(19, 29));
  BOOST_CHECK(!m1.is_allocated(20, 30));
  BOOST_CHECK(!m1.is_allocated(20, 30));


}


