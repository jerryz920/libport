
#include "metadata.h"
#include "assert.h"
#include "log.h"

int main() {
  auto id = "127.0.0.1:100-200";
  latte::MetadataServiceClient c("http://localhost:10011", "127.0.0.1:100-200");
  latte::setloglevel(LOG_DEBUG);

  auto b1 = c.post_new_principal(id,
      "p1", "192.168.0.1", 1000, 2000,
      "image1", "no_config");
  auto b2 = c.post_new_principal(id,
      "p2", "192.168.0.2", 1000, 2000,
      "image2", "no_config");

  c.post_new_image(id,
      "image1", "git://github.com/jerryz920/mysql", "0x50123A4",
      "no_config");
  c.post_new_image(id,
      "image2", "git://github.com/jerryz920/spark", "0x1235612",
      "no_config");

  c.endorse_image(id, "image1", "happy", "no_config");
  c.endorse_image(id, "image2", "unhappy", "no_config");
  c.post_object_acl(id, "object1", "happy");
  c.post_object_acl(id, "object2", "unhappy");
  assert(c.has_property(id, "192.168.0.1", 1005, "happy", b1));
  assert(!c.has_property(id, "192.168.0.1", 1005, "unhappy", b1));
  assert(!c.has_property(id, "192.168.0.1", 999, "happy", b1));
  assert(c.has_property(id, "192.168.0.2", 1005, "unhappy", b2));
  assert(c.can_access(id, "192.168.0.1", 1005, "object1", b1));
  assert(!c.can_access(id, "192.168.0.1", 1005, "object2", b1));
  assert(!c.can_access(id, "192.168.0.1", 999, "object1", b1));
  assert(c.can_access(id, "192.168.0.2", 1005, "object2", b2));
  c.remove_principal(id, 
      "p1", "192.168.0.1", 1000, 2000,
      "image1", "no_config");
  assert(!c.can_access(id, "192.168.0.1", 1005, "object1", b1));
  return 0;
}
