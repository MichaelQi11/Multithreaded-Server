#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include"common.h"
#include"timer.h"

struct thread_data{
    int rank;
    int clientFileDescriptor;
};

char** theArray;
pthread_mutex_t lock;
struct thread_data thread_data_array[COM_NUM_REQUEST];
double *memTime;

void *ServerEcho(void *args)
{
    double start;
    double end;
    struct thread_data *my_data;
    my_data = (struct thread_data *)args;
    int rank = my_data->rank;
    int clientFileDescriptor=my_data->clientFileDescriptor;
    char requestReceive[COM_BUFF_SIZE];

    read(clientFileDescriptor, requestReceive, COM_BUFF_SIZE);
    ClientRequest request;
    ParseMsg(requestReceive, &request);

    char requestString[COM_BUFF_SIZE];

    GET_TIME(start);
    pthread_mutex_lock(&lock);
    if(!request.is_read) {
        setContent(request.msg, request.pos, theArray);
    }
    getContent(requestString, request.pos, theArray);
    pthread_mutex_unlock(&lock);

    GET_TIME(end);
    memTime[rank] = end-start;
    write(clientFileDescriptor, requestString, COM_BUFF_SIZE);

    close(clientFileDescriptor);
}


int main(int argc, char* argv[])
{
    struct sockaddr_in sock_var;
    int serverFileDescriptor=socket(AF_INET,SOCK_STREAM,0);
    int clientFileDescriptor;
    int i;
    pthread_t *t;

    memTime = malloc(COM_NUM_REQUEST*sizeof(double));
    t = malloc(COM_NUM_REQUEST*sizeof(pthread_t));

    if (argc != 4){ 
        fprintf(stderr, "usage: %s <Size of theArray_ on server> <server ip> <server port>\n", argv[0]);
        exit(0);
    }

    int arraySize = strtol(argv[1], NULL, 10);
    theArray = malloc(arraySize*sizeof(char *));
    pthread_mutex_init(&lock, NULL);
    for(int a = 0; a < arraySize; a++) {
        theArray[a] = malloc(COM_BUFF_SIZE*sizeof(char));
        sprintf(theArray[a], "theArray[%d]: initial value", a);
    }

    sock_var.sin_addr.s_addr=inet_addr(argv[2]);
    sock_var.sin_port=strtol(argv[3], NULL, 10);
    sock_var.sin_family=AF_INET;
    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor,COM_NUM_REQUEST); 
        while(1) {
            for(i = 0; i < COM_NUM_REQUEST; i++) {
                memTime[i] = 0;
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
                //printf("Connected to client %d\n",clientFileDescriptor);
                thread_data_array[i].rank = i;
                thread_data_array[i].clientFileDescriptor = clientFileDescriptor;
                pthread_create(&t[i],NULL,ServerEcho,(void *)&thread_data_array[i]);
            }
            for(i = 0; i < COM_NUM_REQUEST; i++) {
                pthread_join(t[i], NULL);
            }
            saveTimes(memTime, COM_NUM_REQUEST);
        }
        
        close(serverFileDescriptor);
    }
    else{
        printf("socket creation failed\n");
    }
    for(int a = 0; a < arraySize; a++) {
        free(theArray[a]);
    }
    pthread_mutex_destroy(&lock);
    free(theArray);
    free(t);
    return 0;
}

