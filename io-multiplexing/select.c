#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>

#define BUFFER_SIZE 4096

int connect_to_host(const char *hostname, const char *port){
    struct addrinfo hints, *res, *p;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname, port, &hints, &res);
    if (status != 0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }
    
    for (p = res; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0){
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen))
        {
            close(sockfd);
            continue;
        }
        
        break;   
    }
    
    freeaddrinfo(res);

    if (p == NULL){
        return -1;
    }

    return sockfd;   
}

int main() {
    return 0;
}