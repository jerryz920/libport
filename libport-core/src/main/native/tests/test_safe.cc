
#include "metadata.h"
#include "log.h"

static std::string safe_url = "";
static const char *IAAS_ID="152.3.145.38:444";
static const char *ATTEST_ID="152.3.145.138:4144";
static const char *OBJ_OWNER = "alice";


static void populate_next_level_inst(const char *speaker, 
    latte::MetadataServiceClient* client,
    const char *id, const char *ip, int port_min, int port_max, const char *image) {
  client->post_new_principal(speaker, id, ip, port_min, port_max, image, "*");
}


static void populate_vm(latte::MetadataServiceClient* client,
    const char *ipbuf, const char *image, const char *image1, const char *image2) {
  static int id = 0;
  char idbuf[128];
  sprintf(idbuf, "vm-%d", id++);
  client->post_new_principal(IAAS_ID, idbuf, ipbuf, 1, 65535, image, "*");
  client->endorse_attester(IAAS_ID, image, "*");
  for (int i = 0; i < 2; i++) {
    sprintf(idbuf, "ctn-%d", id++);
    char speaker[128];
    sprintf(speaker, "%s:%d-%d", ipbuf, 1, 65535);
    latte::MetadataServiceClient vm(safe_url, speaker);
    int pmin = 1000 * i;
    int pmax = 1000 * i + 100;
    populate_next_level_inst(speaker, &vm, idbuf, ipbuf, pmin, pmax, image1);
    vm.endorse_attester(speaker, image1, "*");
    for (int j = 0; j < 2; j++) {
      char ctnspeaker[128];
      sprintf(ctnspeaker, "%s:%d-%d", ipbuf, pmin, pmax);
      latte::MetadataServiceClient ctn(safe_url, ctnspeaker);
      sprintf(idbuf, "proc-%d", id++);
      populate_next_level_inst(ctnspeaker, &ctn, idbuf, ipbuf, pmin + j * 10, pmin + j * 10 + 10,
        image2);
    }
  }
}

static void inline test_create_principal() {
  latte::MetadataServiceClient client(safe_url, IAAS_ID);
  populate_vm(&client, "10.10.0.1", "tapcon", "spark", "pio");
  populate_vm(&client, "10.10.0.2", "tapcon", "spark", "pio");
  populate_vm(&client, "10.10.0.3", "tapcon", "spark", "pio");
  populate_vm(&client, "10.10.0.4", "tapcon", "spark", "pio");
  latte::MetadataServiceClient att_client(safe_url, ATTEST_ID);
  printf("attest of 10.10.0.3:%d --- %s\n", 20000, att_client.attest(ATTEST_ID, "10.10.0.3", 20000, "").c_str());
  printf("attest of 10.10.0.3:%d --- %s\n", 1001, att_client.attest(ATTEST_ID, "10.10.0.3", 1001, "").c_str());
  printf("attest of 10.10.0.3:%d --- %s\n", 1090, att_client.attest(ATTEST_ID, "10.10.0.3", 1090, "").c_str());
}

static void inline test_post_object_acl() {
  latte::MetadataServiceClient client(safe_url, OBJ_OWNER);
  client.post_object_acl(OBJ_OWNER, "objxxx", "git://spark");
  client.post_object_acl(OBJ_OWNER, "objxxy", "git://docker");
  client.post_object_acl(OBJ_OWNER, "objxxz", "git://pio");
}

static void inline test_endorse_image() {
  latte::MetadataServiceClient client(safe_url, IAAS_ID);
  client.endorse_image(IAAS_ID, "tapcon", "git://docker", "*");
  client.endorse_image(IAAS_ID, "spark", "git://spark", "*");
  client.endorse_image(IAAS_ID, "pio", "git://pio", "*");
  client.endorse_attester(IAAS_ID, "tapcon", "*");
  client.endorse_attester(IAAS_ID, "spark", "*");
  latte::MetadataServiceClient att_client(safe_url, ATTEST_ID);
  printf("attest of 10.10.0.3:%d --- %d\n", 20000, att_client.has_property(ATTEST_ID, "10.10.0.3", 20000, "git://docker", ""));
  printf("attest of 10.10.0.3:%d --- %d\n", 1001, att_client.has_property(ATTEST_ID, "10.10.0.3", 1001, "git://pio", ""));
  printf("attest of 10.10.0.3:%d --- %d\n", 1090, att_client.has_property(ATTEST_ID, "10.10.0.3", 1090, "git://spark", "")); }

static std::string obj(const char *name) {
  return "alice:" + std::string(name);
}

static void inline test_access() {
  latte::MetadataServiceClient att_client(safe_url, ATTEST_ID);
  int ports[] = {20000, 1001, 1090};
  const char *objs[] = {"objxxx", "objxxy", "objxxz"};
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      printf("attesting 10.10.0.3:%d, result: %d\n", ports[i],
          att_client.can_access(ATTEST_ID, "10.10.0.3", ports[i], obj(objs[j]), ""));
    }
  }
}

static bool check_prefix(const char *str, const char *prefix) {
  int i;
  for (i = 0; *str && *prefix; i++) {
    if (str[i] != prefix[i])
      break;
  }
  return prefix[i] == 0;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "usage: test_safe http://safe_ip:safe_port\n");
    return 1;
  }

  if (!check_prefix(argv[1], "http://")) {
    fprintf(stderr, "safe url must start with http://, actual %s\n", argv[1]);
    return 2;
  }
  safe_url = argv[1];
  if (argc > 2) {
    latte::setloglevel(LOG_DEBUG);
  }

  printf("test create principal\n");
  test_create_principal();
  printf("test post object acl\n");
  test_post_object_acl();
  printf("test post endorse image\n");
  test_endorse_image();
  printf("test access \n");
  test_access();
  return 0;
}





