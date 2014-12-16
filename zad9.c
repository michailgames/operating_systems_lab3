#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>

#define N 32
#define MAXMSG 32
#define FREE 1
#define ACQUIRED 0
#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(1); }

int sockfd;
char *filename = "the_server";
char buffer[MAXMSG+1];

void sigint_handler() {
    close(sockfd);
    remove(filename);
    printf("Socket closed.\n");
    exit(0);
}

int main() {
    signal(SIGINT, sigint_handler);
    int id_pool[N];
    for(int i = 0; i < N; i++) {
        id_pool[i] = FREE;
    }
    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        ERROR("Creating datagram socket error");
    }
    struct sockaddr_un addr;
    bzero(&addr, sizeof(addr));
    strcpy(addr.sun_path, filename);
    addr.sun_family = AF_UNIX;
    if(bind(sockfd, (struct sockaddr *) &addr, strlen(addr.sun_path) +
            sizeof(addr.sun_family)) < 0) {
        ERROR("Binding socket error");
    }
    while(1) {
        struct sockaddr_un client_address;
        socklen_t len = sizeof(client_address);
        int n = recvfrom(sockfd, buffer, MAXMSG, 0, (struct sockaddr *) &client_address, &len);
        if(n < 0) {
            ERROR("recvfrom error");
        }
        printf("receive!\n");
        buffer[n] = 0;
        printf("%s\n", buffer);
    }
    return 0;
}
