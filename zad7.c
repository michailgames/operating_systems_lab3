#include <stdio.h> // Used only to convert PID to string
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BYTES_TO_READ 128
#define ERROR(str) { write(STDERR_FILENO, str, sizeof(str)); exit(1); }

char *filename = "./Makefile";
char buffer[BYTES_TO_READ+64];
int sockfd[2];

void some_file_action(int fd) {
    bzero(&buffer, sizeof(buffer));
    sprintf(buffer, "PID: %d", getpid());
    strcat(buffer, "\n");
    int bytes_read = read(fd, buffer + strlen(buffer), BYTES_TO_READ);
    if(bytes_read < 0) {
        ERROR("File read error!");
    }
    strcat(buffer, "\n");
    int n = strlen(buffer);
    if(write(STDOUT_FILENO, buffer, n) != n) {
        ERROR("Write to stdout error!");
    }
}

int main() {
    if(socketpair(AF_UNIX, SOCK_DGRAM, 0, sockfd) < 0) {
        ERROR("Creating socket pair error!\n");
    }
    pid_t child = fork();
    
    if(child) { // parent
        close(sockfd[0]);
        
        int fd = open(filename, O_RDONLY);
        if(fd < 0) {
            ERROR("Opening a file error!\n");
        }
        some_file_action(fd);
        
        struct msghdr msg;
        struct cmsghdr *cmsg;
        bzero(&msg, sizeof(msg));
        char cmsg_buffer[CMSG_SPACE(sizeof(fd))];
        msg.msg_control = cmsg_buffer;
        msg.msg_controllen = sizeof(cmsg_buffer);
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
        memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
        msg.msg_controllen = cmsg->cmsg_len;
        if(sendmsg(sockfd[1], &msg, 0) < 0) {
            ERROR("sendmsg error!");
        }
        sleep(1);
    }
    
    else { // child
        close(sockfd[1]);
        int fd;
        
        struct msghdr msg;
        struct cmsghdr *cmsg;
        bzero(&msg, sizeof(msg));
        char cmsg_buffer[CMSG_SPACE(sizeof(fd))];
        msg.msg_control = cmsg_buffer;
        msg.msg_controllen = sizeof(cmsg_buffer);
        if(recvmsg(sockfd[0], &msg, 0) < 0) {
            ERROR("recvmsg error!");
        }
        cmsg = CMSG_FIRSTHDR(&msg);
        if(cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS) {
            ERROR("No file descriptor received!");
        }
        memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
        
        some_file_action(fd);
    }
    
    return 0;
}
