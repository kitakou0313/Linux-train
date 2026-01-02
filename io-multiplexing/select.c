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

long long get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

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

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    const char *hostname = "example.com";
    const char *port = "80";

    int sockfd = connect_to_host(hostname, port);
    if (sockfd < 0)
    {
        /* code */
        return 1;
    }

    set_nonblocking(sockfd);

    const char *http_request = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "User-Agent: select-demo/1.0\r\n"
        "\r\n";
    
    ssize_t sent = send(
        sockfd,
        http_request,
        strlen(http_request),
        0
    );

    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int total_received = 0;
    int iterations = 0;

    while (1)
    {
        /* code */
        iterations++;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        long long start_time = get_time_us();
        int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        long long elapsed = get_time_us() - start_time;

        printf("select() returned: %d (elapsed: %.3f ms)\n", ret, elapsed / 1000.0);

        if (ret < 0)
        {
            /* code */
            break;
        }else if (ret == 0)
        {
            /* code */
            break;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            printf("→ Socket is READY for reading\n");
            
            ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            
            if (received > 0) {
                buffer[received] = '\0';
                total_received += received;
                
                printf("→ Received %zd bytes (total: %d bytes)\n", received, total_received);
                
                // Print first 200 characters of this chunk
                int print_len = received < 200 ? received : 200;
                printf("\n--- Response Data (first %d bytes) ---\n", print_len);
                for (int i = 0; i < print_len; i++) {
                    putchar(buffer[i]);
                }
                if (received > 200) {
                    printf("\n... (%zd more bytes in this chunk) ...\n", received - 200);
                }
                printf("\n--- End of chunk ---\n");
                
            } else if (received == 0) {
                printf("→ Connection closed by server (EOF)\n");
                break;
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recv");
                    break;
                } else {
                    printf("→ Would block (no data available yet)\n");
                }
            }
        }
        
        if (iterations >= 20) {
            printf("\nReached maximum iterations, stopping.\n");
            break;
        }
    }
    close(sockfd);
    return 0;
}