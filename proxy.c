#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "kheader.h" 

#define MULTITHREAD 1


int doGet(char req[], int writesock)
{
  int sockfd, portno, n, flag;
  struct sockaddr_in serv_addr;
  struct in_addr *pptr;
  struct hostent *server;

  char buffer[256];
  /* Extract host and port from HTTP Request */
  char hoststring[LEN_MSG];
  int i, j;
  /* Parsing the request string */
  for(i=0; i<strlen(req); i++)
  {
    if(req[i] == 'H' && req[i+1] == 'o' && req[i+2] == 's' && req[i+3] == 't')
    {
      for(j=i+6; req[j] != '\r'; j++)
      {
        hoststring[j-i-6] = req[j];
      }
      hoststring[j] = '\0';
      break;
    }
  }
  printf("\nHost extracted : '%s'\n", hoststring);
  /* default port */
  portno = 80;

  /* Create a socket point */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
    perror("Error opening socket\n");
    exit(1);
  }
  server = gethostbyname(hoststring);
  if (server == NULL)
  {
    fprintf(stderr, "No such host\n");
    exit(0);
  }
  printf("\nConnected to host\n");

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  pptr = (struct in_addr  *)server->h_addr;
  bcopy((char *)pptr, (char *)&serv_addr.sin_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  /* Connect to server */
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
  {
    perror("Error in connecting to server\n");
    exit(1);
  }

  /* Message to be sent to the server */
  /* printf("message to server : "); */
  bzero(buffer, 256);
  /* fgets(buffer, 255, stdin); */

  /* Send message to server */
  n = write(sockfd, req, strlen(req));
  if(n < 0)
  {
    perror("Error writing to socket\n");
    exit(1);
  }

  /* Read server response */
  bzero(buffer, 256);
  flag = 1;
  printf("\nreading server response\n");
  while( (n = read(sockfd, buffer, 255) > 0))
  {
    if(flag)
    {
      printf("%s", buffer);
      flag = 0;
    }
    i = write(writesock, buffer, strlen(buffer));
  }
  close(sockfd);
  return n;
}


int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, clilen;
  char buffer[LEN_BUF];
  char msg[LEN_MSG];
  struct sockaddr_in serv_addr, cli_addr;
  int n, flag, i, pid;

  /* Command line arguments */
  if(argc < 2)
  {
    fprintf(stderr, "usage : %s <port>\n", argv[0]);
    exit(0);
  }

  /*call to socket() */
  sockfd=socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
    perror("Error opening socket\n");
    exit(1);
  }

  /*initialize socket structure*/
  bzero((char *) &serv_addr, sizeof(serv_addr)); /* set serv_addr to zeros*/
  portno = atoi(argv[1]);
  printf("Starting proxy on %d\n", portno);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind the host address */
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("Error on binding\n");
    exit(1);
  }

  /* start listening for clients */
  listen(sockfd, 5);
  clilen = sizeof(cli_addr);
  printf("Proxy running...\nHave fun ! - kh\n");

  /* accept actual connection from client */
  while(1)
  {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if(newsockfd < 0)
    {
      perror("Error on accept\n");
      exit(1);
    }

    /* if connection is established, get request */
    bzero(buffer, LEN_BUF);
    bzero(msg, LEN_MSG);
    printf("\nMessage:\n");
    flag = 0;
    while( (n = read(newsockfd, buffer, LEN_BUF-1)) > 0)
    {
      printf("%s", buffer);
      strcat(msg, buffer);
      /* check for \n\n */
      for(i = 0; i < strlen(msg); i++)
      {
        if(msg[i] == '\r')
        {
          if(msg[i+1] == '\n' && msg[i+2] == '\r' && msg[i+3] == '\n')
          {
            printf("\nHTTP Request accepted\n");
            flag = 1;
            break;
          }
        }
      }
      if(flag == 1)
        break;
    }

    /* Send HTTP Request to server */
    if(flag == 1)
    {
#ifdef MULTITHREAD
      pid = fork();
      if(pid == 0)
      {
#endif
        n = doGet(msg, newsockfd);
#ifdef MULTITHREAD
        printf("New thread %d created\n");
      }
#endif
    }
    else
    {
      n = write(newsockfd, "HTTP/1.0 400 Bad Request\n\n", 26);
      if(n < 0)
      {
        perror("Error writing to socket\n");
        exit(1);
      }
    }
#ifdef MULTITHREAD
    if(pid == 0)
    {
      printf("the thread %d fulfilled its destiny successfully\n", getpid());
      exit(0);
    }
#endif
  }
  return 0;
}
