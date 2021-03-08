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

typedef struct{
    int reader;
    int writer;
    pthread_cond_t read_process;
    pthread_cond_t write_process;
    int pending_writer;
    pthread_mutex_t rwlock;
}rw_lock;

struct thread_data{
    int rank;
    int clientFileDescriptor;
};

void rwlock_init(rw_lock *l) {
    l->reader = 0;
    l->writer = 0;
    l->pending_writer = 0;
    pthread_cond_init(&(l->read_process), NULL);
    pthread_cond_init(&(l->write_process), NULL);
    pthread_mutex_init(&(l->rwlock), NULL);
}

void rwlock_rlock(rw_lock *l) {
    pthread_mutex_lock(&(l->rwlock));
    while((l->pending_writer) > 0 || (l->writer) > 0) {
        pthread_cond_wait(&(l->read_process), &(l->rwlock));
    }
    l->reader++;
    pthread_mutex_unlock(&(l->rwlock));
}

void rwlock_wlock(rw_lock *l) {
    pthread_mutex_lock(&(l->rwlock));
    while((l->writer > 0) || (l->reader > 0)) {
        l->pending_writer++;
        pthread_cond_wait(&(l->write_process), &(l->rwlock));
        l->pending_writer--;
    }
    l->writer++;
    pthread_mutex_unlock(&(l->rwlock));
}

void rwlock_unlock(rw_lock *l) {
    pthread_mutex_lock(&(l->rwlock));
    if(l->writer > 0) {
        l->writer--;
    } else if(l->reader > 0) {
        l->reader--;
    }
    pthread_mutex_unlock(&(l->rwlock));

    if(l->reader > 0) {
        pthread_cond_broadcast(&(l->read_process));
    } else if((l->reader == 0) && (l->pending_writer > 0)) {
        pthread_cond_signal(&(l->write_process));
    } else if((l->reader == 0) && (l->pending_writer == 0)) {
        pthread_cond_broadcast(&(l->read_process));
    }
}

char** theArray;
rw_lock rwlocks;
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
    if(!request.is_read) {
        rwlock_wlock(&rwlocks);
        setContent(request.msg, request.pos, theArray);
        getContent(requestString, request.pos, theArray);
        rwlock_unlock(&rwlocks);
    }
    else{
        rwlock_rlock(&rwlocks);
        getContent(requestString, request.pos, theArray);
        rwlock_unlock(&rwlocks);
    }
    
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
    
    rwlock_init(&rwlocks);

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
    free(theArray);

    free(t);
    return 0;
}

