/*
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<argp.h>
#include<stdbool.h>

/*
 -s
 -r
 -i (send only)
 -p
 -f (send required)
 */
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 1337

int nscp_args(int argc, char ** argv, char ** file, char **ip, int * port);

int main(int argc, char ** argv) {

    char * file = NULL;
    char * ip = DEFAULT_IP;
    int port = DEFAULT_PORT;

    if (nscp_args(argc, argv, &file, &ip, &port)) {
        return 1;
    }

    // send stuff
    if (file) {
        printf("sending %s to %s on port %d\n", file, ip, port);
    // receive stuff
    } else {
        printf("receiving on port %d\n", myip, port);
    }

    return EXIT_SUCCESS;
}

int nscp_args(int argc, char ** argv, char ** file, char **ip, int * port) {
    struct argp_option[5] options = {
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
            NULL,
            0,
            "The file to send",
            0
        },
        {
            "ip",
            (int)'i',
            NULL,
            0,
            "The ip to send to",
            0
        },
        {
            "port",
            (int)'p',
            NULL,
            OPTION_ARG_OPTIONAL | ,
            "The port send to or receive on",
            0
        }
    };

    error_t arg_parser(int key, char *arg, struct argp_state *state) {
        // return ARG_ERR_UNKNOWN
        int sender = 2;
        switch(key){
            case 's':
                if (!sender) {
                    return ARG_ERR_UNKNOWN;
                }
                sender = 1;
                break;
            case 'r':
                if (sender == 1) {
                    return ARG_ERR_UNKNOWN;
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
                *port = atoi(arg);
                break;
            default:
                return ARG_ERR_UNKNOWN;
        }
        return 0;
    };

    struct the_argp = {
        &options,
        arg_parser,
        NULL,           // args_doc
        "nscp\n\
         not secure copy\n\
         \v\
         e.g. on one machine,\n\
         nscp -s -f myfile -i 192.168.0.5 -p 9001\n\
         ",

    };

    if(argp_parse((const struct argp *)&the_argp, argc, argv, arg_flags, arg_index, NULL)) {
        return 1;
    }
}
