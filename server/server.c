#include <asm-generic/socket.h>
#include <ctype.h>
#include <limits.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 10 // queue for accept

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

void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds)
{
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
