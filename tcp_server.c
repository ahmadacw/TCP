#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <signal.h>

// MINIMAL ERROR HANDLING FOR EASE OF READING

int table[95];
int proccessing = 0;

int getLength(int socket)
{
  int BytesToRead = sizeof(int);
  int returnValue = 0;
  char *buff = (char *)&returnValue;
  while (BytesToRead>0)
  {
    int bytesRead = read(socket, buff, BytesToRead);
    if (bytesRead < 1)
    {
      fprintf(stderr, "Error : read returned %d or less bytes. %s",bytesRead, strerror(errno));
      if (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE && bytesRead != 0)
      { // unexpected error
        exit(1);
      }
      return -1;
    }

    BytesToRead = -bytesRead;
  }
  return returnValue;
}
int ReadClient(int socket, int *length)
{
  int numberOfBytes = getLength(socket);
  *length=numberOfBytes;
  int origin = numberOfBytes;
  int printable = 0;
  char *buff = (char *)malloc(sizeof(char) * numberOfBytes/4+1);
  memset(buff, 0, sizeof(char)*numberOfBytes);
  char* copy=buff;
  while (numberOfBytes)
  {
    int bytesRead = read(socket, copy, numberOfBytes);
    if (bytesRead < 1)
    {
      fprintf(stderr, "Error : read returned %d bytes. %s",bytesRead, strerror(errno));
      if (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE && bytesRead != 0)
      { // unexpected error
        exit(1);
      }
      return -1;
    }
    numberOfBytes -= bytesRead;
    copy += bytesRead;
  }
  printable=0;
  for (int i = 0; i < origin/4; i++)
  {
    if (31 < buff[i] && buff[i] < 127)
    {
      table[buff[i] - 32]++;
      printable++;
    }
  }
  buff[origin]='\0';
  free(buff);
  return printable;
}
void myHandler(int signal)
{
  if(proccessing==0){ // the server is waiting on accept
    for (int i = 0; i < 95; i++)
    { // finishing up
     printf("char '%c': %u times\n", i + 32, table[i]);
    }
    exit(0);
  }
  // the server is proccesing some client and needs to finish it
  proccessing = -1;
}

int main(int argc, char *argv[])
{
  int listenfd = -1;
  int connfd = -1;
  int streamSize = 0;
  int clientResault;
  struct sigaction sa;
  if (argc < 2)
  {
    fprintf(stderr, "Error : wrong number of arguments.\n");
    return 1;
  }

  sigemptyset(&sa.sa_mask);
  sa.sa_handler=myHandler;
  sa.sa_flags=SA_RESTART|SA_NOCLDSTOP;
  if(sigaction(SIGINT,&sa,NULL)<0){
    fprintf(stderr,"Error: sigaction failed. %s",strerror(errno));
    return 1;
  }
  struct sockaddr_in serv_addr;
  struct sockaddr_in my_addr;
  struct sockaddr_in peer_addr;
  socklen_t addrsize = sizeof(struct sockaddr_in);

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0){
    fprintf(stderr, "Error : with socket creation. %s \n", strerror(errno));
    return 1;
  }
  memset(&serv_addr, 0, addrsize);

  serv_addr.sin_family = AF_INET;
  // INADDR_ANY = any local machine address
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));
  if (0 != bind(listenfd,(struct sockaddr *)&serv_addr,addrsize)){
    fprintf(stderr, "\n Error : Bind Failed. %s \n", strerror(errno));
    return 1;
  }
  printf("Done bind \n");
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
    fprintf(stderr, "Error: setsockopt failed. %s\n", strerror(errno));
    return 1;
  }

  if (0 != listen(listenfd, 10)){
    fprintf(stderr, "\n Error : Listen Failed. %s \n", strerror(errno));
    return 1;
  }
  while (1 && proccessing == 0){
    connfd = accept(listenfd,(struct sockaddr *)&peer_addr,&addrsize);
    if (connfd < 0){
      fprintf(stderr, "\n Error : Accept Failed. %s \n", strerror(errno));
      return 1;
    }
    if(proccessing!=-1)
      proccessing = 1;
    clientResault = ReadClient(connfd, &streamSize);
    if (clientResault == -1){
      if (proccessing != 1)
        break;
      proccessing = 0;
      continue;
    }
    int bytesToWrite =sizeof(int);
    char * buff =(char *) &clientResault;
    while (bytesToWrite){
      int bytesWritten = write(connfd, buff, bytesToWrite);
       if (bytesWritten < 1)
       {
         fprintf(stderr, "Error : write returned %d bytes. %s", bytesWritten, strerror(errno));
         if (errno != ETIMEDOUT && errno != ECONNRESET && errno != EPIPE && bytesWritten != 0)
         { // unexpected error
           exit(1);
         }
         return -1;
       }
      bytesToWrite -= bytesWritten;
      buff += bytesWritten;
    }
    if (proccessing != -1) // checking for a signal
        proccessing = 0;
  }
  close(connfd);
  for (int i = 0; i < 95; i++)
  { // finishing up
    printf("char '%c': %u times\n", i + 32, table[i]);
  }
  return 0;
}