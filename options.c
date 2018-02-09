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
            "send",                                 // name
            (int)'s',                               // char
            NULL,                                   // arg
            0,                                      // flags
            "Send a file to a waiting receiver",    // doc
            0                                       // group
        },
        {
            "receive",
            (int)'r',
            NULL,
            0,
            "Wait to receive one or more files from one or more senders",
            0
        },
        {
            "file",
            (int)'f',
            "FILENAME",
            0,
            "The file to send. Only valid with -s",
            0
        },
        {
            "ip",
            (int)'i',
            "IP_ADDRESS",
            0,
            "The ip to send to. Only valid with -s",
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

    int sender = 2;

    error_t arg_parser(int key, char *arg, struct argp_state *state) {

        switch(key){
            case 's':
                if (sender == 0) {
                    fprintf(stderr, "-s and -r are mutually exclusive!");
                    return EINVAL;
                }
                sender = 1;
                break;
            case 'r':
                if (sender == 1) {
                    fprintf(stderr, "-s and -r are mutually exclusive!");
                    return EINVAL;
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
        "nscp\n"
            "Not secure copy. Utility for copying files across the internet.\n"
            "Use -s to send or -r to receive files."
            "\v"
            "e.g. Send \"myfile\" to a receiver listening on port 9001 at 192.168.0.5\n"
            "nscp -s -f myfile -i 192.168.0.5 -p 9001\n"
            "\n"
            "e.g. Listen for and receive files on port 9001\n"
            "nscp -r -p 9001\n",
        NULL,
        NULL,
        NULL
    };

    unsigned arg_flags = 0;

    if(argp_parse((const struct argp *)&the_argp, argc, argv, arg_flags, NULL, NULL)) {
        return -1;
    }

    if (sender == 2) {
        fprintf(stderr, "Please use -s or -r to specify sender or receiver\n");
        argp_help((const struct argp *)&the_argp, stderr, ARGP_HELP_USAGE, "");
    }

    return sender;
}
