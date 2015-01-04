#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#define MAXMSG 8
#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(1); }

int sockfd;
char *filename = "./the_server";
char buffer[MAXMSG+1];

void sigint_handler() {
    close(sockfd);
    printf("\nSocket closed.\n");
    exit(0);
}

int main() {
    signal(SIGINT, sigint_handler);
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        ERROR("Creating datagram socket error");
    }
    // auto-bind client socket
    struct sockaddr_un me;
    me.sun_family = AF_UNIX;
    if(bind(sockfd, (void*)&me, sizeof(short)) < 0) {
        ERROR("connect error");
    }
    // server address
    struct sockaddr_un addr;
    bzero(&addr, sizeof(addr));
    strcpy(addr.sun_path, filename);
    addr.sun_family = AF_UNIX;
    socklen_t len = sizeof(addr);
    printf("Usage: -1 to acquire id, <n> to release id number n.\n");
    while(1) {
        printf("> ");
        fgets(buffer, MAXMSG + 1, stdin);
        int n = strlen(buffer);
        if(sendto(sockfd, buffer, n, 0,
                (struct sockaddr *) &addr, len) != n) {
            ERROR("sendto error");
        }
        bzero(&buffer, sizeof(buffer));
        struct sockaddr_un server_address;
        socklen_t len = sizeof(server_address);
        n = recvfrom(sockfd, buffer, MAXMSG, 0,
                (struct sockaddr *) &server_address, &len);
        if(n < 0) {
            ERROR("recvfrom error");
        }
        printf("%s\n", buffer);
    }
}
