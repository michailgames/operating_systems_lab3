#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>

#include <unistd.h>

#define N 32
#define FREE 1
#define ACQUIRED 0
#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(1); }

int main() {
    int id_pool[N];
    for(int i = 0; i < N; i++) {
        id_pool[i] = FREE;
    }
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        ERROR("Creating datagram socket error");
    }
    struct sockaddr_un addr;
    strcpy(addr.sun_path, "~/my_little_server");
    addr.sun_family = AF_UNIX;
    if(bind(sockfd, (struct sockaddr *) &addr, strlen(addr.sun_path) +
            sizeof(addr.sun_family)) < 0) {
        ERROR("Binding socket error");
    }
    sleep(20);
    return 0;
}
