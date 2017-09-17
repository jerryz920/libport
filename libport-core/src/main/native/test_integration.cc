
//// Simulate a process call the port library, for some new process and call it in the forked process
// This test should be executed on a modified kernel to work out.
#include "libport.h"
#include <string>
#include <stdio.h>
#include <thread>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>
#include "utils.h"

namespace {
std::string server_url = "http://10.10.1.1:10011";
uint32_t local_abac_port = 10013;
std::string object_to_attest = "integrate_object";
}

#define FAIL(msg) fprintf(stderr, "error at line %d: %s\n", __LINE__, (msg))
#define SUSPEND(msg) {fprintf(stderr, "error at line %d: %s\n", __LINE__, (msg)); exit(1);}

int wait_principal(pid_t child) {
  int status;
  int ret = 0;
  if (waitpid(child, &status, 0) < 0) {
    perror("waitpid");
    ret = -1;
  }
  if (WIFEXITED(status)) {
    printf("%d exit!\n", child);
    ret = WEXITSTATUS(status);
  } else {
    ret = -2;
  }
  //::delete_principal(child);
  return ret;
}


int create_server_socket(int local_port) {

  int s = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(local_port),
    .sin_addr = {INADDR_ANY},
  };

  int val = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
    perror("setsockopt");
  }
  int flag = fcntl(s, F_GETFL, 0);
  if (fcntl(s, F_SETFL,  flag | O_NONBLOCK) < 0) {
    perror("fcntl");
  }

  if (bind(s, (struct sockaddr*) (&addr), sizeof(addr)) < 0) {
    perror("bind");
    SUSPEND("fail to allocate socket");
  }

  if (listen(s, 100) < 0) {
    perror("listen");
    SUSPEND("fail to listen on port");
  }

  return s;
}

bool client_try_access(int local_port) {

  int c = socket(AF_INET, SOCK_STREAM, 0);
  auto myip = latte::utils::get_myip();
  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(local_port),
    .sin_addr = {inet_addr(myip.c_str())},
  };

  struct sockaddr_in my_addr;
  socklen_t my_addr_len = sizeof(my_addr);
  if (connect(c, (struct sockaddr*) (&addr), sizeof(addr)) < 0) {
    /// This way we only quit the client process, server needs
    // to capture the status
    perror("connect");
    SUSPEND("fail to connect to server ");
  }
  getsockname(c, (struct sockaddr*) &my_addr, &my_addr_len);
  fprintf(stderr, "client socket seen port: %d\n", ntohs(my_addr.sin_port));

  char buf = 0;
  while (true) {
    int ret = recv(c, &buf, 1, 0);
    if (ret == 0 || (ret < 0 && errno == EAGAIN)) {
      continue;
    }
    if (ret == 1) {
      break;
    } else {
      perror("recv");
      SUSPEND("error receiving message from server\n");
    }
  }
  close(c);
  return buf == 1;
}


int accept_child_task() {
  /// fork twice and do new principal
  if (client_try_access(local_abac_port)) {
    return 0;
  }
  return 1;
}

int reject_normal_child_task() {
  /// connect to local_abac_port
  if (client_try_access(local_abac_port)) {
    return 0;
  }
  return 1;
}

int attester_child_task() {
  /// works as attester, create a new attester with accepted image and try.
  ::libport_init(server_url.c_str(), "/tmp/libport-integration-child.json");
  pid_t child;
  fprintf(stderr, "entering attester\n");
  if ((child = fork()) == 0) {
    /// child
    usleep(100000);
    exit(accept_child_task());

  } else {
    fprintf(stderr, "before create first principal \n");
    ::create_principal(child, "image_accept", "config_accept", 10);
    int ret = wait_principal(child);
    if (ret != 0) {
      fprintf(stderr, "ChildAttester: child that should be accepted returns %d\n", ret);
      FAIL("principal failure\n");
      return 1;
    }
  }

  // fork and simulate bad principal
  if ((child = fork()) == 0) {
    /// child
    usleep(100000);
    exit(reject_normal_child_task());

  } else {
    fprintf(stderr, "before create second principal \n");
    ::create_principal(child, "image_reject", "config_reject", 10);
    int ret = wait_principal(child);
    if (ret != 1) {
      fprintf(stderr, "ChildAttester: child that should fail returns %d\n", ret);
      FAIL("principal failure\n");
      return 1;
    }
  }
  return 0;
}



int main(int argc, char **argv) {

  if (argc > 1) {
    server_url = argv[1];
  }
  ::libport_set_log_level(LOG_DEBUG);

  if (mkdir("/tmp/libport-integration/", 0700) < 0 && errno != EEXIST) {
    perror("mkdir");
    SUSPEND("can not create monitor directory");
  }

  if (::libport_init(server_url.c_str(), "/tmp/libport-integration.json")) {
    SUSPEND("fail to init the libport");
  }
  bool terminate = false;
  printf("original terminate %p\n", &terminate);

  auto listener = std::thread([&terminate]() {
      int server_s = create_server_socket(local_abac_port);
      printf("captured terminate %p\n", &terminate);
      while (!terminate) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int c = accept(server_s, (struct sockaddr*)&client_addr, &client_addr_len);
        if (c < 0 && errno == EAGAIN) {
          usleep(100000);
          continue;
        }
        std::thread([c,client_addr]() {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);
            bool allow_access = ::attest_principal_access(client_ip, client_port,
                object_to_attest.c_str());
            char buf = allow_access? 1: 0;
            while (true) {
              int ret = send(c, &buf, 1, MSG_NOSIGNAL);
              if ((ret < 0 && errno == EAGAIN) || ret == 0) {
                continue;
              }
              break;
            }
            close(c);
            }).detach();
      }
      close(server_s);
      });

  /// start a passive port on localhost:local_abac_port
  // the thread only attests if access to object 'xxx' is permitted

  // Create new images
  if (::create_image("image_accept", "git://accept_image", "rev1", "config1")) {
    SUSPEND("fail to create image_accept");
  }
  if (::create_image("image_reject", "git://accept_reject", "rev2", "config1")) {
    SUSPEND("fail to create image_accept");
  }

  // Endorse image with new properties
  if (::endorse_image("image_accept", "accept_property")) {
    SUSPEND("fail to endorse image property");
  }
  // Create new object acls
  if (::post_object_acl(object_to_attest.c_str(), "accept_property")) {
    SUSPEND("fail to endorse object acl");
  }
  
  // fork and create new principal
  pid_t child;
  if ((child = fork()) == 0) {
    /// child
    usleep(100000);
    exit(accept_child_task());

  } else {
    fprintf(stderr, "launched child %d\n", child);
    ::create_principal(child, "image_accept", "config_accept", 100);
    int ret = wait_principal(child);
    if (ret != 0) {
      fprintf(stderr, "child that should be accepted returns %d\n", ret);
      FAIL("principal failure\n");
    }
  }

  // fork and simulate bad principal
  if ((child = fork()) == 0) {
    /// child
    usleep(100000);
    exit(reject_normal_child_task());

  } else {
    ::create_principal(child, "image_reject", "config_reject", 100);
    int ret = wait_principal(child);
    if (ret != 1) {
      fprintf(stderr, "child that should fail returns %d\n", ret);
      FAIL("principal failure\n");
    }
  }

  fprintf(stderr, "------start of attester task -----\n");
  // fork and simulate bad principal
  if ((child = fork()) == 0) {
    /// child
    attester_child_task();
    exit(0);

  } else {
    ::create_principal(child, "image_accept", "config_accept", 100);
    int ret = wait_principal(child);
    if (ret != 0) {
      fprintf(stderr, "MainProcess: attester child returns %d\n", ret);
      FAIL("main process principal failure\n");
    }
  }
  // simulate restart, resume from last config check point

  terminate = true;
  listener.join();

  return 0;

}
