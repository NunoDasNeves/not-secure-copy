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

int nscp_send(char * file, char * ip, char * port) {
    printf("sending %s to %s on port %s\n", file, ip, port);

    // open file and check its size etc

    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Couldn't open file!\n");
        return 1;
    }

    struct stat filestat = {0};
    if (fstat(fd, &filestat)) {
        fprintf(stderr, "Couldn't stat file!\n");
        return 1;
    }

    off_t filesize = filestat.st_size;

    // get address info for receiver n whatnot
    int status;
    struct addrinfo hints;
    struct addrinfo *result; 			// will point to the results
    memset(&hints, 0, sizeof hints); 	// make sure the struct is empty

    hints.ai_family = AF_UNSPEC; 		// don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; 	// TCP stream sockets

    if ((status = getaddrinfo(ip, port, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    // create socket
    int sock;
    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock < 0) {
        fprintf(stderr, "couldn't create socket\n");
        return 1;
    }

    // connect to receiver
    if (connect(sock, result->ai_addr, result->ai_addrlen)) {
        fprintf(stderr, "couldn't connect to receiver\n");
        return 1;
    }

    size_t curr_to_send = 8;
    int curr_sent = 0;
    size_t filenamelen = strlen(file);

    // send header (filenamesize, filesize)
    uint32_t header[2];
    // convert to network byte order
    header[0] = htonl(filenamelen);
    header[1] = htonl(filesize);
    while (curr_to_send) {
        curr_sent = send(sock, (const void *)header, curr_to_send, 0);
        if (curr_sent == -1) {
            perror("header");
            return 1;
        }
        curr_to_send -= curr_sent;
    }
    printf("done sending header\n");

    // send filename
    size_t left_to_send = filenamelen;
    curr_to_send = filenamelen > MAX_MTU ? MAX_MTU : filenamelen;
    char * filenameptr = file;
    printf("sending name \"%s\"\n", filenameptr);
    while (curr_to_send) {
        curr_sent = send(sock, (const void *)filenameptr, curr_to_send, 0);
        if (curr_sent == -1) {
            perror("send filename");
            return 1;
        }
        filenameptr += curr_sent;
        left_to_send -= curr_sent;
        curr_to_send = left_to_send > MAX_MTU ? MAX_MTU : left_to_send;
    }

    // send file
    left_to_send = filesize;
    curr_to_send = filesize > MAX_MTU ? MAX_MTU : filesize;
    char filebuffer[MAX_MTU] = {0};
    while (left_to_send) {
        ssize_t curr_read = 0;
        while (curr_read < curr_to_send) {
            ssize_t this_read = read(fd, (void *)filebuffer, curr_to_send);
            if (this_read < 0) {
                fprintf(stderr, "error reading file\n");
            }
            curr_read += this_read;	
            // TODO what to do if this_read always 0?
        }

        curr_sent = 0;
        char * filebufferptr = filebuffer;
        while(curr_sent < curr_to_send) {
            int this_sent = send(sock, (const void *)filebufferptr, curr_to_send, 0);
            if (this_sent == -1) {
                fprintf(stderr, "error sending file data\n");
                return 1;
            }
            curr_sent += this_sent;
            filebufferptr += curr_sent;
        }
        left_to_send -= curr_sent;
        curr_to_send = left_to_send > MAX_MTU ? MAX_MTU : left_to_send;
    }

    freeaddrinfo(result); // free the linked-list of addrinfo

    return 0;
}

