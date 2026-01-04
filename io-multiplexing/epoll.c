#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#define BUFFER_SIZE 4096
#define MAX_EVENTS 10

// Get current time in microseconds
long long get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// Create and connect a socket to the given host and port
int connect_to_host(const char *hostname, const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    
    int status = getaddrinfo(hostname, port, &hints, &res);
    if (status != 0) {
        return -1;
    }
    
    // Try each address until we successfully connect
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            continue;
        }
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        
        break; // Successfully connected
    }
    
    freeaddrinfo(res);
    
    if (p == NULL) {
        return -1;
    }

    return sockfd;
}

// Set socket to non-blocking mode
void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    // Connect to example.com on port 80 (HTTP)
    const char *hostname = "example.com";
    const char *port = "80";
    
    int sockfd = connect_to_host(hostname, port);
    if (sockfd < 0) {
        return 1;
    }

    set_nonblocking(sockfd);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(sockfd);
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        perror("epoll_ctl ADD");
        close(sockfd);
        close(epoll_fd);
        return 1;
    }

    const char *http_request = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "User-Agent: epoll-demo/1.0\r\n"
        "\r\n";

    ssize_t sent = send(sockfd, http_request, strlen(http_request), 0);
    if (sent < 0)
    {
        close(sockfd);
        close(epoll_fd);
        return 1;
    }

    struct epoll_event events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];
    int total_received = 0;
    int iterations = 0;

    while (1)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 10000); // 1

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].events & EPOLLIN)
            {
                ssize_t received = recv(
                    sockfd, buffer, sizeof(buffer)-1, 0
                );
                if (received > 0)
                {
                    /* code */
                    buffer[received] = '\0';
                    total_received += received;

                    int print_len = received < 200 ? received : 200;
                    for (int i = 0; i < print_len; i++) {
                        putchar(buffer[i]);
                    }

                }else if(received == 0){
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, NULL);
                }
                
            }

            if (events[i].events & EPOLLHUP) {
                printf("→ EPOLLHUP: Connection hang up\n");
            }
            
            if (events[i].events & EPOLLERR) {
                printf("→ EPOLLERR: Error condition\n");
            }   
        }
    }
}