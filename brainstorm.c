#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>     //getopt

int main(int argc, char **argv)
{
    int input_options = -1;

    //start program
    while( (input_options = getopt(argc, argv, "hvp:i:c:")) != -1)
    {
        switch(input_options)
        {
            case 'v':   //verbose
                break;
            case 'p':   //port
                break;
            case 'i':   //ip address to connect to
                break;
            case 'c':
                printf("%s:", optarg);
                break;
            default:
            case '?':
            case 'h':   //help
                exit(EXIT_SUCCESS);
        }
    }

    while(optind < argc)
    {
        printf("%s ", argv[optind]);
        optind++;
    }
    return 0;
}
