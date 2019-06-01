/*************************************************************
* Professor:    Jesse Chaney
* Author:		    Joshua Hardy
* Filename:     socket_client.c
* Date Created:	1/6/2018
* Date Modifed: 1/6/2018
**************************************************************/
/*************************************************************
* Lab/Assignment: Lab 9
*   socket Client. Connects to a server and sends or recieves
*    a file. Can also request dir list from server.
* Overview:
*   Client that connects to server to get/put files or DIR
* Input:
*   from stdin
* Output:
*   to stdout
* Usage:
*   -p port#   -i 127.0.0.1  -c GET file_name1 file_name2 ...
*   -c PUT file_name1 file_name2 ...
*   -c DIR
************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //getopt()
#include <string.h> //memset()
#include <signal.h> //SIGINT
#include "socket_client.h"

int main(int argc, char **argv)
{
    int input_options = -1;

    char *remote_ip = NULL;
    int port = -1;
    thread_connect_data_t *thread_data = NULL;

    size_t thread_count = 0;
    pthread_attr_t thread_attributes;

    char *command = NULL;
    command_t cmd_type = -1;

    int i = 0;
    int r_val = 0;

    //start program

    //register control c
    signal(SIGINT, end_client);

    while( (input_options = getopt(argc, argv, "hp:i:c:")) != -1)
    {
        switch(input_options)
        {
            case 'p':   //port
                port = atoi(optarg);
                break;
            case 'i':   //ip address to connect to
                remote_ip = optarg;
                break;
            case 'c':
                command = optarg;
                break;
            default:
            case '?':
            case 'h':   //help
                display_help();
                exit(EXIT_SUCCESS);
        }
    }

    if(port == -1)
    {
        printf("-p # not optional!\n\n");
        exit(0);
    }
    if(remote_ip == NULL)
    {
        //univ of oklahoma
        printf("-i 129.xxx.xxx.xxx not optional!\n\n");
        exit(0);
    }
    if(command == NULL)
    {
        printf("-c not optional!\n\n");
        exit(0);
    }

    //get files on commandline
    cmd_type = get_command_type(command);
    switch(cmd_type)
    {
    case DIR:
        thread_data = malloc(sizeof(thread_connect_data_t));
        memset(&thread_data[0], 0, sizeof(thread_connect_data_t));

        thread_data[0].command = cmd_type;
        thread_data[0].port = port;
        strcpy(thread_data[0].remote_ip, remote_ip);
        thread_count++;
        break;
    case GET:
    case PUT:
        thread_data = malloc(sizeof(thread_connect_data_t));
        memset(&thread_data[0], 0, sizeof(thread_connect_data_t));

        thread_data[0].command = cmd_type;
        thread_data[0].port = port;
        strcpy(thread_data[0].remote_ip, remote_ip);
        thread_count++;
        break;
    default:
        fprintf(stderr, "-c argument takes GET or PUT, followed by file names. Or -c DIR alone.\n");
        exit(0);
        break;
    }

    if(cmd_type == GET || cmd_type == PUT)
    {
        while(optind < argc)
        {
            thread_data = realloc(thread_data, sizeof(thread_connect_data_t) * (thread_count + 1));
            memset(&thread_data[thread_count], 0, sizeof(thread_connect_data_t));

            strcpy(thread_data[thread_count].file_name, argv[optind]);
            thread_data[thread_count].command = cmd_type;
            strcpy(thread_data[thread_count].remote_ip, remote_ip);
            thread_data[thread_count].port = port;

            thread_count++;
            optind++;
        }
    }

    for(i = 0; i < thread_count; i++)
    {
        pthread_attr_init(&thread_attributes);
        pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_JOINABLE);

        r_val = pthread_create(&thread_data[i].thread, &thread_attributes, &thread_start, (void*) &thread_data[i]);
        if(r_val != 0)
        {
            perror("thread_create error");
            break;
        }
    }

    for(i = 0; i < thread_count; i++)
    {
        pthread_join(thread_data[i].thread, NULL);

        if(thread_data[i].command != DIR)
            printf("File %s %s completed.\n", thread_data[i].file_name, (thread_data[i].command == GET) ? "GET" : "PUT");
        else
            printf("Directory request completed.\n");

        free(thread_data[i]);
    }

    //cleanup
    free(thread_data);

    return 0;
}

/**********************************************************************
* Function:         void *thread_start(void *data)
* Purpose:          start a thread which will spawn a connection
                    and send file,get file, or return DIR
* Precondition:     pass thread_connect_data_t with filled in members
* Postcondition:    thread created and job executed.
************************************************************************/
void *thread_start(void *data)
{
    thread_connect_data_t thread_data = *((thread_connect_data_t*) data);
    char server_command_string[MAX_STR] = { 0 };
    int bytes_read = 0;
    int size = 0;

    create_connection(&thread_data);
    if(thread_data.command == PUT)
    {
        //send command
        sprintf(server_command_string, "PUT %s", thread_data.file_name);
        write(thread_data.sock_fd, server_command_string, sizeof(char) * MAX_STR);

        send_file(thread_data.file_name, thread_data.sock_fd);
    }
    else if(thread_data.command == GET)
    {
        //send command
        sprintf(server_command_string, "GET %s", thread_data.file_name);
        write(thread_data.sock_fd, server_command_string, strlen(server_command_string) * sizeof(char));

        get_file(thread_data.file_name, thread_data.sock_fd);
    }
    else if(thread_data.command == DIR)
    {
        //send command
        write(thread_data.sock_fd, "DIR", 3 * sizeof(char));
        printf("DIR command sent\n");

        //go ahead and read data
        while((bytes_read = read(thread_data.sock_fd, server_command_string, MAX_STR * sizeof(char))) > 0)
        {
            server_command_string[bytes_read] = 0;
            printf("%s", server_command_string, bytes_read);
        }

        printf("\n***End Transmission***\n");
    }

    close(thread_data.sock_fd);
    pthread_exit(NULL);
}

/**********************************************************************
* Function:         void get_file(char *file_name, int sock_fd)
* Purpose:          recieves a file from socket
* Precondition:     pass filename and socket
* Postcondition:    file recieved.
************************************************************************/
void get_file(char *file_name, int sock_fd)
{
    int file_out = -1;
    char file_data[MAX_STR];
    int bytes_read = 0;

    file_out = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(file_out > -1)
    {
        while((bytes_read = read(sock_fd, file_data, MAX_STR)) > 0)
            write(file_out, file_data, bytes_read);

        close(file_out);
    }
}

/**********************************************************************
* Function:         void send_file(char *file_name, int sock_fd)
* Purpose:          send a file over a socket
* Precondition:     pass filename and socket
* Postcondition:    file sent.
************************************************************************/
void send_file(char *file_name, int sock_fd)
{
    int file_in = -1;
    char file_data[MAX_STR];
    int bytes_read = 0;

    file_in = open(file_name, O_RDONLY);
    if(file_in > -1)
    {
        while((bytes_read = read(file_in, file_data, MAX_STR)) > 0)
            write(sock_fd, file_data, bytes_read);

        close(file_in);
    }
}

/**********************************************************************
* Function:         void create_connection
                      (thread_connect_data_t *thread_connect)
* Purpose:          creates a connection associated with a thread
* Precondition:     pass in thread_connect_data_t type with filled
                    in information.
* Postcondition:    connection created.
************************************************************************/
void create_connection(thread_connect_data_t *thread_connect)
{
    thread_connect->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(thread_connect->sock_fd < 0)
    {
        perror("Failed to created socket descriptor");
        exit(0);
    }

    memset(&thread_connect->sockaddr, 0, sizeof(thread_connect->sockaddr));
    thread_connect->sockaddr.sin_family = AF_INET;
    thread_connect->sockaddr.sin_port = htons(thread_connect->port);
    inet_pton(AF_INET, thread_connect->remote_ip, &thread_connect->sockaddr.sin_addr);

    if(connect(thread_connect->sock_fd, (struct sockaddr*) &thread_connect->sockaddr, sizeof(thread_connect->sockaddr)) < 0)
    {
        perror("Could not connect");
        exit(0);
    }
}

/**********************************************************************
* Function:         command_t get_command_type(char *command)
* Purpose:          returns enum of command type
* Precondition:     pass in command string *
* Postcondition:    enum returned of (Dir, Get, Put)
************************************************************************/
command_t get_command_type(char *command)
{
    command_t actual_command = -1;

    if(command != NULL)
    {
        if(strcmp(command, "GET") == 0)
            actual_command = GET;
        else if(strcmp(command, "PUT") == 0)
            actual_command = PUT;
        else if(strcmp(command, "DIR") == 0)
            actual_command = DIR;
    }
    return actual_command;
}

/**********************************************************************
* Function:         void end_client(int signo)
* Purpose:          signal handler to capture ctrl+c
* Precondition:     register handler in main
* Postcondition:    signal handler registered
************************************************************************/
void end_client(int signo)
{
    if(signo == SIGINT)
    {
        printf("Client terminating...\n");
        exit(0);
    }
}

/**********************************************************************
* Function:         void display_help(void);
* Purpose:          display help to screen
* Precondition:     none
* Postcondition:    help displayed to screen
************************************************************************/
void display_help(void)
{
    printf("Example: socket_client -i 127.0.0.1 -p 1090 -DIR\n"
           "\t-i ip_address\n"
           "\t-p port\n"
           "\t-c DIR\n"
           "\t-c GET filename\n"
           "\t-c PUT filename\n\n");
}
