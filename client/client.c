#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void handle_server_data(int serverfd)
{
    char data[256];
    int nbytes = recv(serverfd, data, 256, 0);
    if (nbytes == 0)
    {
        printf("server closed\n");
        exit(1);
    }
    else
    {
        fputs(data, stdout);
    }
}

void handle_input(int serverfd)
{
    char data[256];
    fgets(data, 256, stdin);
    send(serverfd, data, 256, 0);
}

void process_connections(int serverfd, struct pollfd *pfds)
{
    if (pfds[0].revents & (POLLIN | POLLOUT))
    {
        handle_server_data(serverfd);
    }

    if (pfds[1].revents & POLLIN)
    {
        handle_input(serverfd);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    int serverfd;
    int yes = 1;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "gai_error: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            continue;
        }

        setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        printf("client: attempting connection to %s\n", s);

        if (connect(serverfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("client: socket");
            close(serverfd);
            continue;
        }

        printf("Connected\n");

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "error getting sokcet\n");
        exit(1);
    }

    struct pollfd pfds[2];
    pfds[0].fd = serverfd;
    pfds[0].events = POLLIN;
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;

    for (;;)
    {
        int poll_count = poll(pfds, 2, -1);

        if (poll_count < 0)
        {
            perror("client: poll");
            exit(1);
        }

        process_connections(serverfd, pfds);
    }

    return 0;
}
