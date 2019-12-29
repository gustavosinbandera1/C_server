#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <hiredis/hiredis.h>
//#include <hiredis/async.h>

void print_usage() {
    printf("./redisClient -h <addressServer> -p <portServer> -k <stringkey> -v <stringData>\n");
    exit(2);
}


int main(int argc, char **argv) {
    if(argc < 5) {
        print_usage();
    }

    int option;
    char *host;
    char* packet;
    char* key;
    double port;

    while((option = getopt(argc, argv, "h:p:-k:-v:")) !=-1) {
        switch (option) {
            case 'h' :
                host = (char*)optarg;
                usleep(100);
                fprintf(stderr, "[C-redis-app] Host redis Server:%s \n",host);
                //printf("HOST:%s", host);
                break;
            case 'p' :
                port = atof(optarg);
                usleep(100);
                fprintf(stderr, "[C-redis-app] Port redis Server: %0.0f\n",port);
                //printf("PORT: %0.0f ", port);
                break;
            case 'k' :
                key = (char*)optarg;
                usleep(100);
                fprintf(stderr, "[C-redis-app] controller key:: %s\n",key);
                break;
            case 'v' :
                packet = (char*)optarg;
                usleep(100);
                fprintf(stderr, "[C-redis-app] controller Packet:: %s\n",packet);
                break;
            default :
                fprintf(stderr, "[C-redis-app] Default options");
                break;
        }
    }



    unsigned int j;
    redisContext *c;
    redisReply *reply;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(host, port, timeout);
    //c = redisConnect(hostname, port);
    if (c == NULL || c->err) {
        if (c) {
            fprintf(stderr,"Redis Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            fprintf(stderr,"Redis Connection error: can't allocate redis context\n");
        }
        exit(1);
    }
    
    /* PING server */
    reply = redisCommand(c,"PING");
    printf("[C-redis-app]PING: %s\n", reply->str);
    freeReplyObject(reply);

    
    reply = redisCommand(c,"SET %s %s", key, packet);
    printf("[C-redis-app]SET: %s\n", reply->str);
    freeReplyObject(reply);
    

    reply = redisCommand(c,"GET %s",key);
    printf("GET Key : %s<--->%s\n",key, reply->str);
    freeReplyObject(reply);

    /* Disconnects and frees the context */
    redisFree(c);

    return 0;
}


/*
gcc -o redisTest redisTest.c -lhiredis
*/



//gcc -o redisTest redisTest.c -lhiredis $(pkg-config --cflags --libs glib-2.0)