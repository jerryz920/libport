
#include "metadata.h"

static std::string safe_url = "";
static const char *IAAS_ID="152.3.145.38:444";
static const char *ATTEST_ID="152.3.145.138:4144";
static const char *OBJ_OWNER = "alice";


static void populate_next_level_inst(latte::MetadataServiceClient* client,
    const char *id, const char *ip, int port_min, int port_max, const char *image) {
  client->post_new_principal(id, ip, port_min, port_max, image, "*");

}


static void populate_vm(latte::MetadataServiceClient* client,
    const char *ipbuf, const char *image, const char *image1, const char *image2) {
  static int id = 0;
  char idbuf[128];
  sprintf(idbuf, "vm-%d", id++);
  client->post_new_principal(idbuf, ipbuf, 1, 65535, image, "*");
  client->endorse_attester(image, "*");
  for (int i = 0; i < 5; i++) {
    sprintf(idbuf, "ctn-%d", id++);
    char speaker[128];
    sprintf(speaker, "%s:%d-%d", ipbuf, 1, 65535);
    latte::MetadataServiceClient vm(safe_url, speaker);
    int pmin = 1000 * i;
    int pmax = 1000 * i + 100;
    populate_next_level_inst(&vm, idbuf, ipbuf, pmin, pmax, image1);
    vm.endorse_attester(image1, "*");
    for (int j = 0; j < 3; j++) {
      char ctnspeaker[128];
      sprintf(ctnspeaker, "%s:%d-%d", ipbuf, pmin, pmax);
      latte::MetadataServiceClient ctn(safe_url, ctnspeaker);
      sprintf(idbuf, "proc-%d", id++);
      populate_next_level_inst(&ctn, idbuf, ipbuf, pmin + j * 10, pmin + j * 10 + 10,
        image2);
    }
  }
}

static void inline test_create_principal() {
  latte::MetadataServiceClient client(safe_url, IAAS_ID);
  populate_vm(&client, "1.1.1.1", "tapcon", "spark", "pio");
  populate_vm(&client, "1.1.1.2", "tapcon", "spark", "pio");
  populate_vm(&client, "1.1.1.3", "tapcon", "spark", "pio");
  populate_vm(&client, "1.1.1.4", "tapcon", "spark", "pio");
  latte::MetadataServiceClient att_client(safe_url, ATTEST_ID);
  printf("attest of 1.1.1.3:%d --- %s\n", 1, att_client.attest("1.1.1.3", 1, "").c_str());
  printf("attest of 1.1.1.3:%d --- %s\n", 1001, att_client.attest("1.1.1.3", 1001, "").c_str());
  printf("attest of 1.1.1.3:%d --- %s\n", 1090, att_client.attest("1.1.1.3", 1090, "").c_str());
}

static void inline test_post_object_acl() {
  latte::MetadataServiceClient client(safe_url, OBJ_OWNER);
  client.post_object_acl("objxxx", "git://spark");
  client.post_object_acl("objxxy", "git://docker");
  client.post_object_acl("objxxz", "git://pio");
}

static void inline test_endorse_image() {
  latte::MetadataServiceClient client(safe_url, IAAS_ID);
  client.endorse_image("tapcon", "git://docker", "*");
  client.endorse_image("spark", "git://spark", "*");
  client.endorse_image("pio", "git://pio", "*");
  client.endorse_attester("tapcon", "*");
  client.endorse_attester("spark", "*");
  latte::MetadataServiceClient att_client(safe_url, ATTEST_ID);
  printf("attest of 1.1.1.3:%d --- %d\n", 1, att_client.has_property("1.1.1.3", 1, "git://docker", ""));
  printf("attest of 1.1.1.3:%d --- %d\n", 1001, att_client.has_property("1.1.1.3", 1, "git://pio", ""));
  printf("attest of 1.1.1.3:%d --- %d\n", 1090, att_client.has_property("1.1.1.3", 1, "git://spark", ""));
}

static std::string obj(const char *name) {
  return "alice:" + std::string(name);
}

static void inline test_access() {
  latte::MetadataServiceClient att_client(safe_url, ATTEST_ID);
  int ports[] = {1, 1001, 1090};
  const char *objs[] = {"objxxx", "objxxy", "objxxz"};
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      printf("attesting 1.1.1.3:%d, result: %d\n", ports[i],
          att_client.can_access("1.1.1.3", ports[i], obj(objs[j]), ""));
    }
  }
}


int main(int argc, char **argv) {
  test_create_principal();
  test_post_object_acl();
  test_endorse_image();
  test_access();
  return 0;
}



