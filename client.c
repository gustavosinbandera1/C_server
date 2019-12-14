#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> // for inet_addr
#include <string.h>
#include <zconf.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>
//#define SIGHUP  1   /* Hangup the process */ 
//#define SIGINT  2   /* Interrupt the process */ 
//#define SIGQUIT 3   /* Quit the process */ 
//#define SIGILL  4   /* Illegal instruction. */ 
//#define SIGTRAP 5   /* Trace trap. */ 
//#define SIGABRT 6   /* Abort. */


void print_usage() {
    printf("./client -h <addressServer> -p <portServer> \n");
    exit(2);
}


void connect_to_server(char* host, double port, int *socketHandler, int timeout) {
    int sock;
    struct sockaddr_in server;
    int connection;

    struct timeval tv;
    tv.tv_sec = 30;       /* Timeout in seconds */
   
    //create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    printf("creando socket %d: ",sock); 
    if (sock == -1) {
       printf("[C] could not create socket");
    }
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
    
    //Connect to remote server
    connection = connect(sock, (struct sockaddr *) &server, sizeof(server));
    printf("connectio: %d", connection);
    if (connection == 0) {
        printf("the port is open");
    }
    if (connection < 0) {
        perror("[C] Connect failed. Error");
        //return 1;
        //kill(pid, SIGINT); //if connection is bad kill this process
        exit(1);
    }
     puts("[C] Connected to server\n");
     *socketHandler = sock;

}

void *sendMessage(void *sock_desc) {
    char message[200];
    printf("%s","> ");
    //Send some data
    while (1) {
        memset(message,'\0',sizeof(char*));
        scanf("%[^\n]%*c", message);
       
        if(strcmp (message, "") != 0) {
            printf("el mensaje %s",&message[0]);
            if (send(*((int *) sock_desc), message, strlen(message) + 1, 0) < 0) {
                puts("Send failed");
            }else {
                printf("\n[C] cerrando socket");
                exit(0);//after send message close application
            }
            printf("\n%s","> ");
        } else {  
            printf("plese send any message\n");
            scanf("%c", message);//clear the input error
            //fflush(stdin); //I not recommend this
            printf("%s","> ");
        }
    }
}

void *receiveMessage(void *sock_desc) {
    while (1) {
        char server_reply[200];
        int x = recv(*((int *) sock_desc), server_reply, 2000, 0);
        printf("valor de x = %d", x);
        if(x == 0) {
            exit(0); // normal exit timeout
            return 0;
        }
        if(x < 0) {
            puts("[C] recv failed");
            exit(2);
        }
        if(strcmp (server_reply, "") != 0) {
        //Receive a reply from the server
            printf("[C] Controller message: ");
            printf("\033[32;1m %s \033[0m\n", server_reply);
        }
    }
}

int main(int argc, char **argv) {
    if(argc < 3) {
        print_usage();
    }
    int *new_sock;
    new_sock = malloc(1 * sizeof(int*));
    pid_t pPid = getppid();
    pid_t pid = getpid();
    printf("My process ID : %d\n", getpid());
    usleep(10000);
    printf("My parent's ID: %d\n", getppid());
    int option;
    char *host;
    double port;

    while((option = getopt(argc, argv, "h:p:")) !=-1) {
        switch (option) {
            case 'h' :
                host = (char*)optarg;
                usleep(100);
                fprintf(stdout, "[C] Host Server:%s \n",host);
                break;
            case 'p' :
                port = atof(optarg);
                usleep(100);
                fprintf(stdout, "[C] Port Server: %0.0f\n",port);
                break;
            default :
                fprintf(stderr, "[C] Default options");
                break;
        }
    }
    connect_to_server(host, port, new_sock,10);
    //keep communicating with server
    pthread_t send_thread, receive_thread;
    pthread_create(&send_thread, NULL, sendMessage, (void *) new_sock);
    pthread_create(&receive_thread, NULL, receiveMessage, (void *) new_sock);
   
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);
    //close(sock);
    return 0;
}

