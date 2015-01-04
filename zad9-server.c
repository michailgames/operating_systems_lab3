#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#define N 8
#define MAXMSG 8
#define FREE 1
#define ACQUIRED 0
#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(1); }

int sockfd;
char *filename = "./the_server";
char buffer[MAXMSG+1];
int id_pool[N];

void sigint_handler() {
    close(sockfd);
    remove(filename);
    printf("\nSocket closed.\n");
    exit(0);
}

int acquire() {
    for(int i = 0; i < N; i++) {
        if(id_pool[i] == FREE) {
            id_pool[i] = ACQUIRED;
            return i+1;
        }
    }
    return 0;
}

bool release(int id) {
    if(id < 1 || id > N) {
        return false;
    }
    if(id_pool[id-1] == ACQUIRED) {
        id_pool[id-1] = FREE;
        return true;
    }
    return false;
}

int main() {
    signal(SIGINT, sigint_handler);
    for(int i = 0; i < N; i++) {
        id_pool[i] = FREE;
    }
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
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
        int n = recvfrom(sockfd, buffer, MAXMSG, 0,
                (struct sockaddr *) &client_address, &len);
        if(n < 0) {
            ERROR("recvfrom error");
        }
        buffer[n] = 0;
        int action = atoi(buffer);
        int response = -1;
        if(action == -1) {
            response = acquire();
        }
        else if(action > 0) {
            response = release(action);
        }
        bzero(&buffer, sizeof(buffer));
        sprintf(buffer, "%d", response);
        n = strlen(buffer);
        if(sendto(sockfd, buffer, n, 0,
                (struct sockaddr *) &client_address, len) != n) {
            ERROR("sendto error");
        }
    }
    return 0;
}
