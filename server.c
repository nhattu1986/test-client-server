#include <sys/socket.h>
#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>

#define MSGSIZE 1024
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1


static struct argp_option options[] = {
    {"fork",	'f', 0,      0,  "allow app go background" },
    {"host",	'h', "HOST",      0,  "host to listen" },
    {"port",	'p', "PORT",      0,  "port to listen" },
    { 0 }
};

struct arguments
{
    char *host;
    char *port;
    int fork;
};
      
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'f':
      arguments->fork = 1;
      break;
    case 'h':
        arguments->host = arg;
        break;
    case 'p':
        arguments->port = arg;
        break;


    case ARGP_KEY_END:
      if (state->arg_num > 3)
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


static void daemonize(void)
{
    pid_t pid, sid;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}


int main(int argc, char** argv)
{
    struct arguments arguments;
    int portno;
    int sockfd;
    int clientlen;
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    struct hostent *hostp;
    char buf[MSGSIZE];
    char *hostaddrp;
    int optval;
    int n;
    int ret;

    arguments.fork = 0;
    arguments.host = "0.0.0.0";
    arguments.port = "8199";

    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    printf("fork setting: %d\nhost: %s\nport: %s\n", arguments.fork, arguments.host, arguments.port);
    
    if (arguments.fork == 1)
    {
        daemonize();
    }

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
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    serveraddr.sin_port = htons((unsigned short)portno);


    /*bind*/

    ret = bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (ret < 0)
    {
        printf("error bind\n");
    }

    clientlen = sizeof(clientaddr);
    while (1) 
    {
        memset(buf, 0, MSGSIZE);
        n = recvfrom(sockfd, buf, MSGSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);
        if (n < 0)
        {
            printf("error recv\n");
        }
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
        {
            error("ERROR on gethostbyaddr");
        }
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
        {
            error("ERROR on inet_ntoa\n");
        }
        printf("server received datagram from %s (%s)\n", 
                                        hostp->h_name, hostaddrp);
        printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* 
     * sendto: echo the input back to the client 
     */
        n = sendto(sockfd, buf, strlen(buf), 0, 
            (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
        {
            error("ERROR in sendto");
        }

    }

    return 0;
}