#include <sys/socket.h>
#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>

#define MSGSIZE 1024


static struct argp_option options[] = {
    {"host",	'h', "HOST",      0,  "host to listen" },
    {"port",	'p', "PORT",      0,  "port to listen" },
    { 0 }
};

struct arguments
{
    char *host;
    char *port;
};
      
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'h':
        arguments->host = arg;
        break;
    case 'p':
        arguments->port = arg;
        break;


    case ARGP_KEY_END:
      if (state->arg_num >= 2)
        /* Not enough arguments. */
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, 0, 0 };

void error(char *msg) {
  perror(msg);
  exit(1);
}




int main(int argc, char** argv)
{
    struct arguments arguments;
    int portno;
    int sockfd;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    struct hostent *hostp;
    char buf[MSGSIZE];
    char *hostaddrp;
    int optval;
    int n;
    int ret;

    struct timeval timeout;

    fd_set readsock;
    int maxfd;

    arguments.host = "127.0.0.1";
    arguments.port = "8199";

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    printf("host: %s\nport: %s\n", arguments.host, arguments.port);
    
    portno = atoi(arguments.port);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("error opening socket\n");
    }
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    ret = inet_aton(arguments.host, &serveraddr.sin_addr);
    if (ret == 0)
    {
        printf("invalid ip address: %s\n using 0.0.0.0", arguments.host);
        exit(1);
    }
    serveraddr.sin_port = htons((unsigned short)portno);



    serverlen = sizeof(serveraddr);
    while (1) 
    {
        FD_ZERO(&readsock);

        memset(buf, 0, MSGSIZE);
        printf("Please enter msg: ");
        fgets(buf, MSGSIZE, stdin);
        
        n = sendto(sockfd, buf, strlen(buf), 0, 
            (struct sockaddr *) &serveraddr, serverlen);
        if (n < 0) 
        {
            error("ERROR in sendto");
        }
        
        FD_SET(sockfd, &readsock);
        maxfd = sockfd;
        ret = select(maxfd+1, &readsock, NULL, NULL, &timeout);
        if (ret == -1)
        {
            printf("timeout receive response\n");
        }
        else if (ret)
        {
            printf("Got data\n");

            n = recvfrom(sockfd, buf, MSGSIZE, 0, (struct sockaddr*)&serveraddr, &serverlen);
            if (n < 0)
            {
                printf("error recv\n");
            }
            else
            {
                printf("recv %s\n", buf);
            }

        }
        else
        {
            printf("no data\n");
        }

    }

    return 0;
}