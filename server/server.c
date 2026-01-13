#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <ctype.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

// totally not needed argument checker for this little demo project
void check_args(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    char *endptr;
    long temp_port = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0')
    {
        fprintf(stderr, "invalid character found in port: %c\n", *endptr);
        exit(1);
    }

    if (temp_port < 0)
    {
        fprintf(stderr, "port can't be negative number\n");
        exit(1);
    }

    if (temp_port > USHRT_MAX)
    {
        fprintf(stderr, "port can't be bigger then 2^16\n");
        exit(1);
    }

    while (isspace(*argv[1]))
    {
        argv[1]++;
    }
}

int get_listener_socket(char *port)
{
    int listener;
    int yes = 1;
    int rv;

    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0)
    {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = ai->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, ai->ai_protocol);
        if (listener == -1)
        {
            continue;
        }

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        return -1;
    }

    freeaddrinfo(ai);

    if (listen(listener, 10) == -1)
    {
        return -1;
    }

    return listener;
}

void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size)
{
    if (*fd_count == *fd_size)
    {
        *fd_size *= 2;
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;
    (*pfds)[*fd_count].revents = 0;

    (*fd_count)++;
}

void del_from_pfds(struct pollfd *pfds, int pfd_i, int *fd_count)
{
    pfds[pfd_i] = pfds[*fd_count - 1];
    (*fd_count)--;
}

const char *inet_ntop2(void *addr, char *buf, size_t size)
{
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family)
    {
    case AF_INET:
        sa4 = addr;
        src = &(sa4->sin_addr);
        break;
    case AF_INET6:
        sa6 = addr;
        src = &(sa6->sin6_addr);
        break;
    default:
        return NULL;
    }

    return inet_ntop(sas->ss_family, src, buf, size);
}

void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds)
{
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    int newfd;
    char remoteIP[INET6_ADDRSTRLEN];

    addrlen = sizeof remoteaddr;
    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

    if (newfd == -1)
    {
        perror("accept");
    }
    else
    {
        add_to_pfds(pfds, newfd, fd_count, fd_size);

        printf("pollserver: new connection from %s on socket %d\n", inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP),
               newfd);
    }
}

void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i)
{
    char buf[256];

    int nbytes = recv(pfds[*pfd_i].fd, buf, sizeof buf, 0);

    int sender_fd = pfds[*pfd_i].fd;

    if (nbytes <= 0)
    {
        if (nbytes == 0)
        {
            printf("pollserver: socket %d hung up\n", sender_fd);
        }
        else
        {
            perror("recv");
        }

        close(pfds[*pfd_i].fd);
        del_from_pfds(pfds, *pfd_i, fd_count);
        (*pfd_i)--;
    }
    else
    {
        printf("pollserver: recv from fd %d: %.*s", sender_fd, nbytes, buf);
        for (int i = 0; i < *fd_count; i++)
        {
            int dest_fd = pfds[i].fd;

            if (dest_fd != listener && dest_fd != sender_fd)
            {
                if (send(dest_fd, buf, nbytes, 0) == -1)
                {
                    perror("send");
                }
            }
        }
    }
}

void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds)
{
    for (int i = 0; i < *fd_count; i++)
    {

        if ((*pfds)[i].revents & (POLLIN | POLLHUP))
        {

            if ((*pfds)[i].fd == listener)
            {
                handle_new_connection(listener, fd_count, fd_size, pfds);
            }

            else
            {
                handle_client_data(listener, fd_count, *pfds, &i);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    check_args(argc, argv);
    char *port = argv[1];

    int listener;

    int fd_size = 5;
    int fd_count = 0;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    listener = get_listener_socket(port);

    if (listener == -1)
    {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    pfds[0].fd = listener;
    pfds[0].events = POLLIN;

    fd_count = 1;

    puts("pollserver: waiting for connections...");

    for (;;)
    {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        process_connections(listener, &fd_count, &fd_size, &pfds);
    }

    return 0;
}
