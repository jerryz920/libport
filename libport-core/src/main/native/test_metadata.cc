
#include "metadata.h"
#include "assert.h"
#include "log.h"

int main() {
  latte::MetadataServiceClient c("http://localhost:10011", "");
  latte::setloglevel(LOG_DEBUG);

  c.post_new_principal(
      "p1", "192.168.0.1", 1000, 2000,
      "image1", "no_config");
  c.post_new_principal(
      "p2", "192.168.0.2", 1000, 2000,
      "image2", "no_config");

  c.post_new_image(
      "image1", "git://github.com/jerryz920/mysql", "0x50123A4",
      "no_config");
  c.post_new_image(
      "image2", "git://github.com/jerryz920/spark", "0x1235612",
      "no_config");

  c.endorse_image("image1", "happy");
  c.endorse_image("image2", "unhappy");
  c.post_object_acl("object1", "happy");
  c.post_object_acl("object2", "unhappy");
  assert(c.has_property("192.168.0.1", 1005, "happy"));
  assert(!c.has_property("192.168.0.1", 1005, "unhappy"));
  assert(!c.has_property("192.168.0.1", 999, "happy"));
  assert(c.has_property("192.168.0.2", 1005, "unhappy"));
  assert(c.can_access("192.168.0.1", 1005, "object1"));
  assert(!c.can_access("192.168.0.1", 1005, "object2"));
  assert(!c.can_access("192.168.0.1", 999, "object1"));
  assert(c.can_access("192.168.0.2", 1005, "object2"));
  return 0;
}
