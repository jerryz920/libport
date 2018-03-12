#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int main(int argc, char **argv)
{
  const char *config = "/etc/attguard/config.txt";
  if (argc > 1) {
    config = argv[1];
  }
  if (daemon(0, 0) < 0) {
    perror("creating daemon:");
    exit(1);
  }
  return execlp("attguard", "attguard", config, (char*) NULL);
}
