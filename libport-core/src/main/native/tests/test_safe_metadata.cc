
#include "metadata.h"
#include "assert.h"
#include "log.h"
#include "safe.h"

int main(int argc, char **argv) {
  const char* addr = "http://127.0.0.1:7777";
  char buffer[1024];
  if (argc >= 2) {
    sprintf(buffer, "http://%s:7777", argv[1]);
    addr = buffer;
  }
  latte::MetadataServiceClient c(addr, IAAS_IDENTITY);
  latte::setloglevel(LOG_DEBUG);

  auto b1 = c.post_new_principal("p1", "192.168.0.1", 1000, 2000,
      "image1", "no_config");
  auto b2 = c.post_new_principal("p2", "192.168.0.2", 1000, 2000,
      "image2", "no_config");

  c.post_new_image(
      "image1", "git://github.com/jerryz920/mysql", "0x50123A4",
      "no_config");
  c.post_new_image(
      "image2", "git://github.com/jerryz920/spark", "0x1235612",
      "no_config");
  c.endorse_image("image1", "happy", "no_config");
  c.endorse_image("image2", "unhappy", "no_config");
  latte::MetadataServiceClient alice(addr, "alice");
  alice.post_object_acl("alice:object1", "happy");
  alice.post_object_acl("alice:object2", "unhappy");
  assert(c.has_property("192.168.0.1", 1005, "happy", b1));
  assert(!c.has_property("192.168.0.1", 1005, "unhappy", b1));
  assert(!c.has_property("192.168.0.1", 999, "happy", b1));
  assert(c.has_property("192.168.0.2", 1005, "unhappy", b2));
  assert(c.can_access("192.168.0.1", 1005, "alice:object1", b1));
  assert(!c.can_access("192.168.0.1", 1005, "alice:object2", b1));
  assert(!c.can_access("192.168.0.1", 999, "alice:object1", b1));
  assert(c.can_access("192.168.0.2", 1005, "alice:object2", b2));
  c.remove_principal(
      "p1", "192.168.0.1", 1000, 2000,
      "image1", "no_config");
  assert(!c.can_access("192.168.0.1", 1005, "alice:object1", b1));

  assert(c.has_property("192.168.0.2", 1050, "unhappy", b2));
  latte::MetadataServiceClient p2(addr, "192.168.0.2:1000-2000");
  auto b3 = p2.post_new_principal("pp1", "192.168.0.2", 1050, 1100,
      "image3", "no_config");
  p2.post_new_image("image3", "git://github.com/jerryz920/pio", "0x4567123", "no_config");
  p2.endorse_image("image3", "happy", "no_config");
  assert(p2.has_property("192.168.0.2", 1050, "happy", b3));
  assert(p2.can_access("192.168.0.2", 1050, "alice:object1", b3));

  latte::MetadataServiceClient bob(addr, "bob");
  bob.post_object_acl("bob:objectx", "happy");
  bob.post_object_acl("cindy:objectx", "happy");
  assert(p2.can_access("192.168.0.2", 1050, "bob:objectx", b3));
  assert(p2.can_access("192.168.0.2", 1050, "cindy:objectx", b3));
  latte::MetadataServiceClient cindy(addr, "cindy");
  bob.post_object_acl("objecty", "happy");
  assert(p2.can_access("192.168.0.2", 1050, "objecty", b3));

  return 0;
}
