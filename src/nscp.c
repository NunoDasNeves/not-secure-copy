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

/*
 -s
 -r
 -i (send only)
 -p
 -f (send required)
 */
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "1337"

#define MTU_MAX_SIZE 4096

int nscp_args(int argc, char ** argv, char ** file, char **ip, char **port);

int sender = 2;

int nscp_send(char * file, char * ip, char * port) {
    printf("sending %s to %s on port %s\n", file, ip, port);

	// open file and check its size etc
	
	int fd = open(file, 0);
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
	hints.ai_flags = AI_PASSIVE; 		// fill in my IP for me

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
	uint32_t header[2] = {filenamelen, filesize};
	while (curr_to_send) {
		curr_sent = send(sock, (const void *)header, curr_to_send, 0);
		if (curr_sent == -1) {
			fprintf(stderr, "error sending header\n");
			return 1;
		}
	}

	// send filename
	size_t left_to_send = filenamelen;
	curr_to_send = filenamelen > MTU_MAX_SIZE ? MTU_MAX_SIZE : filenamelen;
	char * filenameptr = file;
	while (curr_to_send) {
		curr_sent = send(sock, (const void *)filenameptr, curr_to_send, 0);
		if (curr_sent == -1) {
			fprintf(stderr, "error sending filename\n");
			return 1;
		}
		filenameptr += curr_sent;
		left_to_send -= curr_sent;
		curr_to_send = left_to_send > MTU_MAX_SIZE ? MTU_MAX_SIZE : left_to_send;
	}

	// send file
	left_to_send = filesize;
	curr_to_send = filesize > MTU_MAX_SIZE ? MTU_MAX_SIZE : filesize;
	char filebuffer[MTU_MAX_SIZE] = {0};
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
		curr_to_send = left_to_send > MTU_MAX_SIZE ? MTU_MAX_SIZE : left_to_send;
	}

	freeaddrinfo(result); // free the linked-list of addrinfo

	return 0;
}

int nscp_receive(char * port) {

	printf("receiving on port %s\n", port);



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

int nscp_args(int argc, char ** argv, char ** file, char **ip, char **port) {
    struct argp_option options[6] = {
        {
            "send",             // name
            (int)'s',           // char
            NULL,               // arg
            0,                  // flags
            "Use this to send", // doc
            0                   // group
        },
        {
            "receive",
            (int)'r',
            NULL,
            0,
            "Use this to receive",
            0
        },
        {
            "file",
            (int)'f',
            "FILENAME",
            0,
            "The file to send",
            0
        },
        {
            "ip",
            (int)'i',
            "IP_ADDRESS",
            0,
            "The ip to send to",
            0
        },
        {
            "port",
            (int)'p',
            "PORT",
            0,
            "The port send to or receive on",
            0
        },
		{0}	// zero entry to terminate the vector
    };

    error_t arg_parser(int key, char *arg, struct argp_state *state) {

        switch(key){
            case 's':
                if (!sender) {
                    return ARGP_ERR_UNKNOWN;
                }
                sender = 1;
                break;
            case 'r':
                if (sender == 1) {
                    return ARGP_ERR_UNKNOWN;
                }
                sender = 0;
                break;
            case 'f':
                *file = arg;
                break;
            case 'i':
                *ip = arg;
                break;
            case 'p':
                *port = arg;
                break;
            default:
                return ARGP_ERR_UNKNOWN;
        }
        return 0;
    };

    struct argp the_argp = {
        options,
        arg_parser,
        NULL,           // args_doc
        "nscp\n\
         not secure copy\n\
         \v\
         e.g. on one machine,\n\
         nscp -s -f myfile -i 192.168.0.5 -p 9001\n\
         ",
		 NULL,
		 NULL,
		 NULL
    };

	unsigned arg_flags = 0;

    if(argp_parse((const struct argp *)&the_argp, argc, argv, arg_flags, NULL, NULL)) {
        return 1;
    }

	return 0;
}
