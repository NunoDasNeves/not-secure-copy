/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <unistd.h>

#include "nscp.h"

// get sockaddr, IPv4 or IPv6:
void * get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int nscp_receive(char * port) {

    printf("receiving on port %s\n", port);

    // the receiver can handle multiple connections, erroring on file

    struct addrinfo hints, *servinfo;
    int sock = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    int success = getaddrinfo(NULL, port, &hints, &servinfo); // node is NULL so we get an address suitable for binding to
    if (success) {
        printf("getaddrinfo failed\n");
        return 1;
    }	

    // create socket and bind to it
    // use the first address info struct that works
    for(struct addrinfo * p = servinfo; p; p = p->ai_next) {

        sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if (sock < 0) {
            perror("socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            return 1;
        }

        if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (sock < 0) {
        printf("none of the address structs could be bound to!\n");
        return 1;
    }

    freeaddrinfo(servinfo); // all done with this structure	

    if (listen(sock, 10) == -1) {
        perror("listen");
        exit(1);
    }
    printf("listening...\n");

    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    while (1) {
        sin_size = sizeof(their_addr);
        int new_fd = accept(sock, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof(s));
        printf("server: got connection from %s\n", s);
        if (!fork()) { // this is the child process
            printf("child process handling connection\n");
            close(sock); // child doesn't need the listener

            // receive the data, parse all that shiz and save it to a file

            // receive file name size and file size
            uint32_t header[2] = {0};
            size_t left_to_receive = 8;
            int amount_received = 0;
            while (left_to_receive) {
                if (!(amount_received = recv(new_fd, header+(8-left_to_receive), left_to_receive, 0))) {
                    fprintf(stderr, "client closed connection!\n");
                    exit(0);
                }
                left_to_receive -= amount_received;
            }
            // convert to host byte order
            for (int i = 0; i < 2; ++i) header[i] = ntohl(header[i]);

            printf("filenamesize = %u, filesize = %u\n", header[0], header[1]);

            // receive file name
            // need a buffer for it
            char * filename = calloc(header[0] + 1, sizeof(char)); // should prob check filename size but meh
            if (!filename) {
                perror("malloc");
                exit(0);
            }
            // pointer in case it takes multiple recvs
            char * filenameptr = filename;
            left_to_receive = header[0];
            amount_received = 0;
            while (left_to_receive) {
                if (!(amount_received = recv(new_fd, filenameptr, left_to_receive, 0))) {
                    fprintf(stderr, "client closed connection!\n");
                    exit(0);
                }
                printf("got %u bytes for filename\n", amount_received);
                printf("got \"%s\" from sender\n", filenameptr);
                left_to_receive -= amount_received;
                filenameptr += amount_received;
            }
            printf("saving file \"%s\" from sender\n", filename);

            // open file to write to
            int fd = open(filename, O_WRONLY|O_CREAT, 0600);
            if (fd<0) {
                perror("open");
                exit(0);
            }

            // receive file
            // need a buffer to chunk the file
            char buffer[MAX_MTU] = {0};
            char * buffptr = buffer; // pointer into the buffer
            // get the size
            left_to_receive = header[1];
            amount_received = 0;

            while(left_to_receive) {
                // reset inner stuff
                size_t left_this_buff = left_to_receive > MAX_MTU ? MAX_MTU : left_to_receive;
                size_t received_this_buff = 0;
                buffptr = buffer;

                // inner loop for batching the incoming jank to the buffer
                // TODO prob don't really need to do it this way; just write() to file every packet prob fine
                while(left_this_buff) {
                    if (!(received_this_buff = recv(new_fd, buffptr, left_this_buff, 0))) {
                        fprintf(stderr, "client closed connection!\n");
                        exit(0);
                    }
                    left_this_buff -= received_this_buff;
                    buffptr += received_this_buff;
                }
                left_to_receive -= received_this_buff;
                amount_received += received_this_buff;

                // write to file
                if (write(fd, buffer, received_this_buff) < received_this_buff) {
                    perror("write");
                    exit(0);
                }
            }

            close(fd);
            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }

    return 0;
}

