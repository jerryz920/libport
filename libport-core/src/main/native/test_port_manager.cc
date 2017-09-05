//#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE PortManagerTest
#include <boost/test/unit_test.hpp>

#include "include/port_manager.h"

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

