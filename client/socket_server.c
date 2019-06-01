/*************************************************************
* Author:		Joshua Hardy
* Filename:     socket_server.c
* Date Created:	1/6/2018
* Date Modifed: 1/6/2018
**************************************************************/
/*************************************************************
*
* Lab/Assignment: Lab 9
*
* Overview:
*	  Server to get/put files and get DIR
* Input:
*	  from stdin
* Output:
*     to stdout
* Usage:
*     run ./socket_server
************************************************************/
#include <stdio.h>
#include <stdlib.h> //strtoull()
#include <unistd.h> //getopt()
#include <string.h> //memset()
#include <signal.h> //SIGINT
#include "socket_server.h"

static int end_of_world;

int main(int argc, char **argv)
{
    int input_options = -1;

    int port = 1090;
    thread_connect_data_t *thread_data = NULL;
    size_t thread_count = 0;

    int listen_fd = -1;
    struct sockaddr_in server_addr;
    socklen_t client_len;

    pthread_attr_t thread_attributes;

    //int i = 0;
    int r_val = 0;

    end_of_world = 0;
    //start program

    //register control c
    signal(SIGINT, end_server);

    while( (input_options = getopt(argc, argv, "p:")) != -1)
    {
        switch(input_options)
        {
            case 'p':   //port
                port = atoi(optarg);
                break;
            default:
            case '?':
            case 'h':   //help
                //display_help();
                exit(EXIT_SUCCESS);
        }
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0)
    {
        perror("Couldn't open listen_fd:");
        exit(0);
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(listen_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("Couldn't open listen_fd failed to bind:");
        exit(0);
    }

    //server start
    while(end_of_world == FALSE)
    {
        thread_data = malloc(sizeof(thread_connect_data_t) * (thread_count + 1));

        //reset thread data each iteration since threads are detached.
        memset(&thread_data[thread_count], 0, sizeof(thread_connect_data_t));
        listen(listen_fd, 5);
        printf("Server listening on port: %i\n", port);

        client_len = sizeof(thread_data[thread_count].client);
        thread_data[thread_count].sock_fd = accept(listen_fd, (struct sockaddr*) &thread_data[thread_count].client, &client_len);

        if(thread_data[thread_count].sock_fd < 0)
        {
            perror("Failed to accept incoming socket:");
            exit(0);
        }

        pthread_attr_init(&thread_attributes);
        pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);

        r_val = pthread_create(&thread_data[thread_count].thread, &thread_attributes, &thread_start, (void*) &thread_data[thread_count]);
        if(r_val != 0)
        {
            perror("thread_create error");
            break;
        }
        
        thread_count++;
    }
    free(thread_data);
    printf("Server shutting down.\n");
    return 0;
}

void *thread_start(void *data)
{
    int bytes_read = 0;

    thread_connect_data_t thread_data = *((thread_connect_data_t*) data);
    char recieved_command_string[MAX_STR] = { 0 };
    
    printf("Accepted connection - spawned thread.\n");
    bytes_read = read(thread_data.sock_fd, recieved_command_string, sizeof(char) * MAX_STR);
    recieved_command_string[bytes_read] = 0;
    recieved_command_string[3] = 0;

    printf("read: %s\n", recieved_command_string);

    if(strstr(recieved_command_string, "GET") != NULL)
    {
        send_file(&recieved_command_string[4], thread_data.sock_fd);
    }
    else if(strstr(recieved_command_string, "PUT") != NULL)
    {
        get_file(&recieved_command_string[4], thread_data.sock_fd);
    }
    else if(strstr(recieved_command_string, "DIR") != NULL)
    {
        FILE *fp;
        fp = popen("ls -lFA", "r");
        while((bytes_read = fread(recieved_command_string, sizeof(char), sizeof(char) * MAX_STR, fp)) > 0);
        {
            write(thread_data.sock_fd, recieved_command_string, strlen(recieved_command_string));
            printf("DIR:\n%s\n", recieved_command_string);
        }
        printf("----END DIR TRANSMISSION----");
        fclose(fp);
    }

    close(thread_data.sock_fd);
    printf("thread/socket closed\n");
    pthread_exit(NULL);
}

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

void end_server(int signo)
{
    if(signo == SIGINT)
    {
        printf("Server terminating...\n");
        exit(0);
    }
}
