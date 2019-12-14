/*
 1. Install hiredis:
    git clone git://github.com/antirez/hiredis.git
    cd hiredis
    make
    make install
 2. Build C program:
    gcc -o redis-key-stats -lhiredis -I/usr/local/include/hiredis redis-key-stats.c
 3. Run program:
    ./redis-key-stats <address> <port>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

int main(int argc, char ** argv)
{
    redisContext * c;
    redisReply * keysReply;
    redisReply * typeReply;
    redisReply * lengthReply;
    long double counts[5];
    long double sums[5];
    char * key;
    unsigned long i;
    
    if(argc<3)
    {
        printf("Usage: %s <address> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    c = redisConnect(argv[1], atoi(argv[2]));
    if (c->err)
    {
        printf("Error: %s\n", c->errstr);
        return EXIT_FAILURE;
    }

    sums[0] = sums[1] = sums[2] = sums[3] = sums[4] = 0;
    counts[0] = counts[1] = counts[2] = counts[3] = counts[4] = 0;

    keysReply = redisCommand(c, "KEYS *");
    for(i=0;i<keysReply->elements;i++)
    {
        key = keysReply->element[i]->str;
        typeReply = redisCommand(c, "TYPE %s", key);

        if(!strcmp(typeReply->str, "string"))
        {
            lengthReply = redisCommand(c, "STRLEN %s", key);
            counts[0]++;
            sums[0] += lengthReply->integer;
        }

        if(!strcmp(typeReply->str, "list"))
        {
            lengthReply = redisCommand(c, "LLEN %s", key);
            counts[1]++;
            sums[1] += lengthReply->integer;
        }

        if(!strcmp(typeReply->str, "set"))
        {
            lengthReply = redisCommand(c, "SCARD %s", key);
            counts[2]++;
            sums[2] += lengthReply->integer;
        }

        if(!strcmp(typeReply->str, "zset"))
        {
            lengthReply = redisCommand(c, "ZCARD %s", key);
            counts[3]++;
            sums[3] += lengthReply->integer;
        }

        if(!strcmp(typeReply->str, "hash"))
        {
            lengthReply = redisCommand(c, "HLEN %s", key);
            counts[4]++;
            sums[4] += lengthReply->integer;
        }

        freeReplyObject(typeReply);
    }
    freeReplyObject(keysReply);

    printf("total count: %d\n", keysReply->elements);

    printf("\nstrings count: %Lf\n", counts[0]);
    printf("strings avg len: %0.2Lf\n", sums[0]==0 ? 0 : (sums[0]/counts[0]));

    printf("\nlists count: %Lf\n", counts[1]);
    printf("lists avg len: %0.2Lf\n", sums[1]==0 ? 0 : (sums[1]/counts[1]));

    printf("\nsets count: %Lf\n", counts[2]);
    printf("sets avg len: %0.2Lf\n", sums[2]==0 ? 0 : (sums[2]/counts[2]));

    printf("\nzsets count: %Lf\n", counts[3]);
    printf("zsets avg len: %0.2Lf\n", sums[3]==0 ? 0 : (sums[3]/counts[3]));

    printf("\nhashes count: %Lf\n", counts[4]);
    printf("hashes avg len: %0.2Lf\n", sums[4]==0 ? 0 : (sums[4]/counts[4]));

    return EXIT_SUCCESS;
}