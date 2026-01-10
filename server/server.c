#include <ctype.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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

int main(int argc, char *argv[])
{
    check_args(argc, argv);
    char *port = argv[1];

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // my ip
    //

    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
    }

    return 0;
}
