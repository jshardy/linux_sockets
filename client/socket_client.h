/*************************************************************
* Professor:    Jesse Chaney
* Author:		    Joshua Hardy
* Filename:     socket_client.h
* Date Created:	1/6/2018
* Date Modifed: 1/6/2018
* For:          Lab9
**************************************************************/

#ifndef SOCKET_CLIENT
#define SOCKET_CLIENT

#include <sys/socket.h> //socket()
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //inet_pton
#include <linux/limits.h> //PATH_MAX
#include <sys/stat.h> //open()
#include <fcntl.h> //O_CREAT
#include <pthread.h>

#define TRUE    1
#define FALSE   0

#define MAX_STR 1024

typedef enum {
    DIR = 1,
    GET = 2,
    PUT = 3
} command_t;

typedef struct thread_connect_data_s
{
    char file_name[PATH_MAX];
    char remote_ip[16];
    int port;
    command_t command;
    struct sockaddr_in sockaddr;
    int sock_fd;
    pthread_t thread;
} thread_connect_data_t;

void *thread_start(void *data);
void create_connection(thread_connect_data_t *thread_connect);
void get_file(char *file_name, int sock_fd);
void send_file(char *file_name, int sock_fd);
command_t get_command_type(char *command);
void end_client(int signo);
void display_help(void);

#endif
