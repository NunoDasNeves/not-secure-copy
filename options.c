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
