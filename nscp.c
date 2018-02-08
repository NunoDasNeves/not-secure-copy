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

/*
 -s
 -r
 -i (send only)
 -p
 -f (send required)
 */
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "1337"

#define MAX_MTU 4096

int sender = 2;

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

int main(int argc, char ** argv) {

    char * file = NULL;
    char * ip = DEFAULT_IP;
    char * port = DEFAULT_PORT;

    if (nscp_args(argc, argv, &file, &ip, &port)) {
        return 1;
    }

    // send stuff
    if (sender == 1) {
		if (!file) {
			printf("please specify a filename with -f\n");
			return 1;
		}

		if (nscp_send(file, ip, port)) {
			printf ("send failed!\n");
			return 1;
		}

    // receive stuff
    } else if (sender == 0) {

		if (nscp_receive(port)) {
			printf ("receive failed!\n");
			return 1;
		}

    } else {
		printf("please use -s or -r to specify sender or receiver\n");
		return 1;
	}

    return EXIT_SUCCESS;
}

