/*
File: client.c
Authors: Aidan Morgan, Kyle Shake, Michael Smith
Course Name: CS371 Project 2

Last Modified: 4/25/18
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>

typedef struct sockaddr_in SA;

void *writemessage(void *clientfd){
    while(1){
        char msgbuf[1024];
        int msgsize;
        fgets(msgbuf, 1024, stdin);
        msgbuf[strcspn(msgbuf, "\n")] = 0;
        msgsize = strlen(msgbuf) + 1;
        write(clientfd, msgbuf, msgsize);
        bzero(msgbuf, sizeof(msgbuf));
    }
}


int main(int argc, char *argv[])
{
    /*Console call check*/
    if(argc != 3){
        fprintf(stderr, "usage: %s <server name> <port>\n", argv[0]);
        exit(1);
    }

    /*Get nickname*/
    char *clname = malloc(21);
    printf("What nickname would you like to use? (20 characters max) \n");
    fgets(clname, 20, stdin);
    clname[strcspn(clname, "\n")] = 0;
    int namelen = strlen(clname)+1;

    /*Initialize variables and assign arg values*/
    int port, clientfd;
    struct hostent *hp;
    struct sockaddr_in serv_addr;
    char *host;
    pthread_t td;

    host = argv[1];
    port = atoi(argv[2]);
    if(port == 0){
        fprintf(stderr, "error: port cannot be %s", argv[2]);
    }

    /* Socket settings */
    if((clientfd=socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "error: socket error");
        exit(-1);
    }

    if((hp = gethostbyname(host)) == NULL){
        fprintf(stderr, "error: get host info error");
        exit(-1);
    } 
    
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy((char *)hp->h_addr_list[0],
	(char *)&serv_addr.sin_addr.s_addr, hp->h_length);

    serv_addr.sin_port = htons(port);

    /* Request for connecting to the server */

    if(connect(clientfd, (SA *)&serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr, "error: connect to host failed");  
    }
    char *hostaddr;
    hostaddr = inet_ntoa(serv_addr.sin_addr);

    printf("Connected to server: %s (%s)\n", host,hostaddr); 
    write(clientfd, clname, namelen); //Send server the name first

    /* Send and receive messages */
    char msgbuf[1024];
    int msgsize;
   
    pthread_create(&td, NULL, &writemessage, (void *)clientfd);

    while((msgsize = read(clientfd, msgbuf, 1024)) > 0){
        printf("%s\n>", msgbuf);
        bzero(msgbuf, sizeof(msgbuf));
    }
    pthread_join(td, NULL);
    return 0;
}
