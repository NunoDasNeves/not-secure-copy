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

int main(int argc, char ** argv) {


    char * filename = NULL;
    char * ip = DEFAULT_IP;
    char * port = DEFAULT_PORT;

    int sender = nscp_args(argc, argv, &filename, &ip, &port);

    // send stuff
    if (sender == 1) {
        if (!filename) {
            printf("please specify a filename with -f\n");
            return 1;
        }

        if (nscp_send(filename, ip, port)) {
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
        return 1;
    }

    return EXIT_SUCCESS;
}

