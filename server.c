/*Name: Aidan Morgan, Kyle Shake, Michael Smith
Class: CS 371
Assignment: Project 2
Date: 2018-04-27*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

//Edit this to change the maximum number of clients the server will accept
#define MAX_CLIENTS 10

static unsigned int num_client = 0;
static int uid = 1;

//client_t definition
//Class is composed of an address, connection info, a user id, and a nick
typedef struct {
    struct sockaddr_in addr;
    int connfd;
    int uid;
    char nick[21];
} client_t;

//Create an array of clients to be filled with connections
client_t *clients[MAX_CLIENTS];

//Adds a client to the queue
void queue_add(client_t *cl){
    int i;
    //Checks every slot until an empty one is found.
    for(i=0;i<MAX_CLIENTS;i++){
        if(!clients[i]){
            clients[i] = cl;
            return;
        }
    }
}

//Deletes a client from the queue after a disconnection
void queue_delete(int uid){
    int i;
    //Finds the client with the appropriate uid and sets their entry to NULL
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            if(clients[i]->uid == uid){
                clients[i] = NULL;
                return;
            }
        }
    }
}

//Sends a message to all clients but the sender
void send_message(char *s, int uid){
    int i;
    for(i=0;i<MAX_CLIENTS;i++){
        if(clients[i]){
            if(clients[i]->uid != uid){
                write(clients[i]->connfd, s, strlen(s));
            }
        }
    }
}

//Gets rid of carriage returns and new lines
void strip_newline(char *s){
    while(*s != '\0'){
        if(*s == '\r' || *s == '\n'){
            *s = '\0';
        }
        s++;
    }
}

//Server connection threads each run one instance of this function
//Handles all client input
void *handle_client(void *arg){
    char buff_out[1024];
    char buff_in[1024];
    int rlen;
    int nickset = 0;

    num_client++;
    client_t *cli = (client_t *)arg;
    
    //Receives input from user until connection is terminated
    while((rlen = read(cli->connfd, buff_in, sizeof(buff_in)-1)) > 0){
        buff_in[rlen] = '\0';
        buff_out[0] = '\0';
        strip_newline(buff_in);
        
        //Ignores an empty buffer
        if(!strlen(buff_in)){
            continue;
        }
    
        //Checks if message is /exit
        if (!nickset){
            strcpy(cli->nick, buff_in);
            nickset = 1;
        }else if(buff_in[0] == '/'){
            char *command;
            command = strtok(buff_in," ");
            if(!strcmp(command, "/exit")){
                break;
            }
        }else{
            //Sends message to all other clients
            sprintf(buff_out, "[%s] %s\r\n", cli->nick, buff_in);
            send_message(buff_out, cli->uid);
        }
    }

    //Closes connection when read fails
    close(cli->connfd);

    //Deletes client and frees memory
    queue_delete(cli->uid);
    free(cli);
    num_client--;
    
    //Ends thread
    pthread_detach(pthread_self());
    return NULL;
}

int main(int argc, char *argv[]){
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t td;

    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    //Ignore pipe signals
    signal(SIGPIPE, SIG_IGN);
    
    /* Bind */
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Socket binding failed");
        return 1;
    }

    /* Listen */
    if(listen(listenfd, 10) < 0){
        perror("Socket listening failed");
        return 1;
    }

    /* Accept clients */
    while(1){
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        //Check if there's space
        if((num_client+1) == MAX_CLIENTS){
            printf("Max clients reached.\n");
            close(connfd);
            continue;
        }

        /* Client settings */
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->addr = cli_addr;
        cli->connfd = connfd;
        cli->uid = uid++;

        /* Add client to the queue */
        queue_add(cli);
        
        /*Create a thread */
        pthread_create(&td, NULL, &handle_client, (void*)cli);
    }
}
