#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

void readFile(char *fileContents, char * filePath,int sizeOfFile){
    FILE* fp = fopen(filePath,"r");
    if(fp!=NULL){
        fread(fileContents,sizeof(char),sizeOfFile/4,fp);
        if(ferror(fp)!=0){
            fprintf(stderr,"Error: reading file. %s",strerror(errno));
            free(fileContents);
            exit(1);
        }
    }
    else{
        fprintf(stderr,"Error: openning file. %s\n",strerror(errno));
        free(fileContents);
        exit(1);
    }      
    fclose(fp);

}
int sendSizeOfMessage(int sockfd, char *filePath)
{
    int sizeOfFile = 0;
    int bytesToWrite;
    char *buff;
    struct stat sb;
    stat(filePath, &sb);
    sizeOfFile = (int)sb.st_size*4;
    bytesToWrite = sizeof(sizeOfFile);
    buff = (char *)(&sizeOfFile);
    while (bytesToWrite)
    {
        int bytesWritten = write(sockfd, buff, bytesToWrite);
        if (bytesWritten <1)
        {
            perror(strerror(errno));
            exit(1);
        }
        bytesToWrite -= bytesWritten;
        buff += bytesWritten;
    }
    return sizeOfFile;
}
int main(int argc, char *argv[])
{
    int sockfd = -1;
    int bytes_read = 0;
    char *fileContents;
    unsigned int sizeOfFile = 0;

    struct sockaddr_in serv_addr; // where we Want to get to
    struct sockaddr_in my_addr;   // where we actually connected through
    struct sockaddr_in peer_addr; // where we actually connected to
    socklen_t addrsize = sizeof(struct sockaddr_in);
    if (argc < 4)
    {
        printf("bad arguments\n");
        exit(1);
    }
    int bytesToWrite=0 ;
    struct stat sb;
    stat(argv[3], &sb);
    sizeOfFile = (int)sb.st_size*4;
    fileContents=(char*) malloc(sizeof(char)*sizeOfFile);
    readFile(fileContents, argv[3],sizeOfFile);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2])); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0){
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        return 1;
    }
    bytesToWrite = sendSizeOfMessage(sockfd, argv[3]);
    if (bytesToWrite < 0){
        close(sockfd);
        perror(strerror(errno));
        exit(1);
    }
    char *currentPosition = fileContents;
    int bytesWritten = 0;
    while (bytesToWrite>0){
        bytesWritten = write(sockfd, currentPosition, bytesToWrite);
        if (bytesWritten == -1)
        {
            close(sockfd);
            perror(strerror(errno));
            exit(1);
        }
        bytesToWrite -= bytesWritten;
        currentPosition += bytesWritten;
    }
    free(fileContents);
    unsigned int output = 0;
    char *buff = (char *)&output;
    int bytesToRead = sizeof(output);
    while (bytesToRead)
    {
        int bytesRead = read(sockfd, buff, bytesToRead);
        if (bytesRead == -1)
        {
            close(sockfd);
            perror(strerror(errno));
            exit(1);
        }
        bytesToRead -= bytesRead;
        buff += bytesRead;
    }
    printf("# of printable characters: %u\n", output);
    close(sockfd); // is socket really done here?
    return 0;
}