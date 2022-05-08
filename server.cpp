#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include "implementMemory.hpp"

  
typedef struct stack{
    char str[1024];
    stack* next;
}stack, *pst;

#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 100
#define BACKLOG 10   // how many pending connections queue will hold
pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;

typedef struct free_block {
    size_t size;
    struct free_block* next;
} free_block;

static free_block free_block_list_head = { 0, 0 };
static const size_t overhead = sizeof(size_t);
static const size_t align_to = 16;
void* my_malloc(size_t size){
    size = (size + sizeof(size_t) + (align_to - 1)) & ~ (align_to - 1);
    free_block* block = free_block_list_head.next;
    free_block** head = &(free_block_list_head.next);
    while (block != 0) {
        if (block->size >= size) {
            *head = block->next;
            return ((char*)block) + sizeof(size_t);
        }
        head = &(block->next);
        block = block->next;
    }

    block = (free_block*)sbrk(size);
    block->size = size;

    return ((char*)block) + sizeof(size_t);
}
void my_free(void* ptr){
    free_block* block = (free_block*)(((char*)ptr) - sizeof(size_t));
    block->next = free_block_list_head.next;
    free_block_list_head.next = block;
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
stack* head = NULL;

void *myThreadFun(void* new_fd){
    char buf[1024]={0};
    int sock=*(int*)new_fd;
    int numbytes;
    while (1) {
        bzero(buf,1024);
        if((numbytes = recv(sock, buf, MAXDATASIZE-1, 0)) ==-1) {
            printf("error\n");
            exit(1);
        }buf[numbytes]='\0';

        if(strstr(buf, "exit")){
            close(sock);
            break;
        }
        else if(strncmp(buf, "PUSH", 4)==0){  
            pthread_mutex_lock(&mylock);
            char str[1024];
            unsigned int i = 0;
            for (unsigned j = 5; j < strlen(buf); i++, j++) {
                str[i] = buf[j];
            }
            str[i] = '\0';
            stack *NewNodePointer = (stack*) my_malloc(sizeof(stack));
            if(!NewNodePointer) return 0;
            strcpy(NewNodePointer->str,str);
            NewNodePointer->next = head;
            head = NewNodePointer;
            send(sock, "thank you for pushing", 25, 0);
            pthread_mutex_unlock(&mylock);
        }

        else if(strstr(buf, "POP")){         
            pthread_mutex_lock(&mylock);
            if(head == NULL) {
                send(sock, "the stack is empty", 25, 0);
            }else{
                stack *PreviousNodePointer = head->next;
                my_free(head);
                head = PreviousNodePointer;
                send(sock, "thank you for popping", 25, 0);
                pthread_mutex_unlock(&mylock);
            }
        }
        else if(strstr(buf, "TOP")){ 
            pthread_mutex_lock(&mylock);
            if (head == NULL){
                send(sock, "the stack is empty", 25, 0);
            }else {
                char ans[1024]="OUTPUT: ";
                strcat(ans, head->str);
                send(sock, ans, strlen(ans), 0);
            }
            pthread_mutex_unlock(&mylock);
        }  
    }
    return NULL;     
}
    
int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    pthread_t tid;

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if(pthread_create(&tid, NULL, &myThreadFun, &new_fd)!=0){
            printf("thread failed");
        }
    }

    return 0;
}