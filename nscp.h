/*
 *
 */

#ifndef NSCP_H
#define NSCP_H

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "1337"

#define MAX_MTU 4096

extern int sender;

int nscp_args(int argc, char ** argv, char ** file, char **ip, char **port);

int nscp_send(char * file, char * ip, char * port);

int nscp_receive(char * port);

#endif
