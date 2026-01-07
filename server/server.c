#include <asm-generic/socket.h>
#include <limits.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 10

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: server <port>\n");
        return 1;
    }

    char *badchar;
    char *port_s = argv[1];
    unsigned long port_l = strtoul(port_s, &badchar, 10);

    if (port_l > USHRT_MAX)
    {
        fprintf(stderr, "Port can't be bigger then 2^16\n");
        return 1;
    }

    if (*badchar != '\0')
    {
        fprintf(stderr, "invalid character: %c\n", *badchar);
        return 1;
    }

    struct addrinfo *servinfo, hints, *p;
    struct sockaddr_storage their_addr;
    int rv;
    int sockfd, new_fd;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    socklen_t sin_size;
    struct sigaction sa;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // host ip

    if ((rv = getaddrinfo(NULL, port_s, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket\n");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("server: setsockopt\n");
            return 1;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind\n");
            continue;
        }
    }

    if (p == NULL)
    {
        fprintf(stderr, "server failed to bind\n");
        return -1;
    }

    freeaddrinfo(servinfo);

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("server: listen\n");
        return -1;
    }

    return 0;
}
