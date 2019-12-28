
#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#endif

#include <string.h>

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <hiredis/hiredis.h>
#include <assert.h>

char *host = "redis";
double port = 6378;
/* one of these created for each message */

struct msg {
	void *payload; /* is malloc'd */
	size_t len;
};

/* one of these is created for each client connecting to us */

struct per_session_data__lws_websocket {
	struct per_session_data__lws_websocket *pss_list; //list of sockets connected
	struct lws *wsi;
	int last; /* the last message number we sent */
};

/* one of these is created for each vhost our protocol is used with */
struct per_vhost_data__lws_websocket {
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;

	struct per_session_data__lws_websocket *pss_list; /* linked-list of live pss*/

	struct msg amsg; /* the one pending message... */
	int current; /* the current message number we are caching */
};

char uCalTCPCheckSum(char *tuBuf, int tuIndex) {
    char tuCnt,tuCheckSum = 0;
    for(tuCnt = 0; tuCnt < tuIndex; tuCnt++) {
        tuCheckSum = tuCheckSum + tuBuf[tuCnt];
    }
    return(tuCheckSum);
}

int uVerifyCheckSum(char *tuBuf) {
    char tuCheckSumIndex = 0, tuCalCheckSum , tuIndex, tuRcvdCheckSum;
    for(tuIndex = 0; tuBuf[tuIndex]; tuIndex++) {
        if(tuBuf[tuIndex] == ',') {
			tuCheckSumIndex = tuIndex + 1;
		}
	}

    tuCalCheckSum = uCalTCPCheckSum(tuBuf,tuCheckSumIndex);
	//fprintf(stderr,"\ninside function Calculated Chksm = %d\n",tuCalCheckSum);

    tuRcvdCheckSum = (char)atoi(&tuBuf[tuCheckSumIndex]);
	//fprintf(stderr,"\ninside function Rcvd Chksm = %d\n",tuRcvdCheckSum);

    if(tuRcvdCheckSum == tuCalCheckSum) {
        return 1;
    } else {
		return 0;
	}

}

char** str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}


char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    //you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char* getRedisCommand(const char* host, const char* port, const char* key, const char* packet) {
	char* temp1 = concat("/usr/local/bin/redisClient -h ",host); //host
	char* temp2 = concat(temp1," -p ");  //port
	char* temp3 = concat(temp2, port);   
	char* temp4 = concat(temp3," -k ");  //key
	char* temp5 = concat(temp4, key);
	char* temp6 = concat(temp5, " -v "); //value 
	char* temp7 = concat(temp6, packet);
	char* temp8 = concat(temp7, " & ");
	fprintf(stderr, "the redis URL connection is: %s\n", temp8);
	return temp8;
}

char* getHttpCommand(const char* host, const char* packet) {
	char* temp1 = concat("wget --post-data ", "packet="); 
	char* temp2 = concat(temp1, packet);  
	char* temp3 = concat(temp2, " ");   
	char* temp4 = concat(temp3, host); 
	char* temp5 = concat(temp4, " ");
	char* temp6 = concat(temp5, " --connect-timeout=5 --tries=2 -O /dev/null &"); //the & is intended to fork the command
	fprintf(stderr, "the POST COMMAND connection is: %s\n", temp6);
    return temp6;
}

void redisNetClose(redisContext *c) {
	if (c && c->fd != REDIS_INVALID_FD) {
		close(c->fd);
		c->fd = REDIS_INVALID_FD;
	}
}
/* destroys the message when everyone has had a copy of it */
static void __lws_websocket_destroy_message(void *_msg) {
	struct msg *msg = _msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
}

static int callback_lws_websocket(struct lws *wsi, enum lws_callback_reasons reason,
	void *user, void *in, size_t len)
{
	struct per_session_data__lws_websocket *pss = (struct per_session_data__lws_websocket *)user;
	struct per_vhost_data__lws_websocket *vhd = (struct per_vhost_data__lws_websocket *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	
	int m; //to check success send message
	char* bufferData;
	char* key;//key to use with redis
	char** tokens;//to store split array message
	const char* redisHost = getenv("redisHost");
    const char* redisPort = getenv("redisPort");
	const char* comlinkHost = getenv("comlinkHost");

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct per_vhost_data__lws_websocket));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);
		break;

	case LWS_CALLBACK_ESTABLISHED:
		/* add ourselves to the list of live pss held in the vhd */
		lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
		pss->wsi = wsi;
		pss->last = vhd->current;
		break;

	case LWS_CALLBACK_CLOSED:
		/* remove our closing pss from the list of live pss */
		lws_ll_fwd_remove(struct per_session_data__lws_websocket, pss_list,
				  pss, vhd->pss_list);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (!vhd->amsg.payload)
			break;

		if (pss->last == vhd->current)
			break;

		/* notice we allowed for LWS_PRE in the payload already */
		m = lws_write(wsi, ((unsigned char *)vhd->amsg.payload) +
			      LWS_PRE, vhd->amsg.len, LWS_WRITE_TEXT);
		if (m < (int)vhd->amsg.len) {
			lwsl_err("ERROR %d writing to ws\n", m);
			return -1;
		}
		pss->last = vhd->current;
		break;

	case LWS_CALLBACK_RECEIVE:
		if (vhd->amsg.payload)
			__lws_websocket_destroy_message(&vhd->amsg);

		vhd->amsg.len = len;
		/* notice we over-allocate by LWS_PRE */
		vhd->amsg.payload = malloc(LWS_PRE + len);
		if (!vhd->amsg.payload) {
			lwsl_user("OOM: dropping\n");
			break;
		}
		bufferData = malloc(len+1);
		memcpy((char *)bufferData,in,len);//message from controller (in)
		bufferData[len]= '\0'; //string terminator
		
		if(uVerifyCheckSum(in)) {
			//fprintf(stderr,"rigth packetes: ");
			//get key to store data inside redis with controller code
			tokens = str_split((char*)in, ',');
			key = *(tokens+1); //device code
			key = concat(key, "_PACKET");
			
			fprintf(stderr,"the key is: %s\n",key);
			char* command = getRedisCommand(redisHost, redisPort, key, bufferData);
			fprintf(stderr, "the command to execute redis is : %s\n", command);
			fprintf(stderr, (char*)system(command));
			 //free memory
			if (tokens) {
				for (int i = 0; *(tokens + i); i++) {
					//fprintf(stderr,">>%s, ", *(tokens + i));
					free(*(tokens + i));
				}
				free(tokens);
			}
			//send notification to comlink server(nodejs code)
			command = getHttpCommand(comlinkHost, bufferData);
			fprintf(stderr,"\nThe command to send data to comlink is: %s\n", command);
			fprintf(stderr, system(command)); 

			//free(command);
			
		} else {
			fprintf(stderr,"bad packet");
			//send NACK
		}
		
		memcpy((char *)vhd->amsg.payload + LWS_PRE, bufferData, len);
		vhd->current++;
		free(bufferData);
	
		
		/*
		 * let everybody know we want to write something on them
		 * as soon as they are ready
		 */
		lws_callback_on_writable(wsi);
		break;

	default:
		break;
	}

	return 0;
}

#define LWS_PLUGIN_PROTOCOL_LWS_WEBSOCKET \
	{ \
		"lws-websocket", \
		callback_lws_websocket, \
		sizeof(struct per_session_data__lws_websocket), \
		128, \
		0, NULL, 0 \
	}

#if !defined (LWS_PLUGIN_STATIC)

/* boilerplate needed if we are built as a dynamic plugin */

static const struct lws_protocols protocols[] = {
	LWS_PLUGIN_PROTOCOL_LWS_WEBSOCKET
};

LWS_EXTERN LWS_VISIBLE int
init_protocol_lws_websocket(struct lws_context *context,
		      struct lws_plugin_capability *c)
{
	if (c->api_magic != LWS_PLUGIN_API_MAGIC) {
		lwsl_err("Plugin API %d, library API %d", LWS_PLUGIN_API_MAGIC,
			 c->api_magic);
		return 1;
	}

	c->protocols = protocols;
	c->count_protocols = LWS_ARRAY_SIZE(protocols);
	c->extensions = NULL;
	c->count_extensions = 0;

	return 0;
}

LWS_EXTERN LWS_VISIBLE int
destroy_protocol_lws_websocket(struct lws_context *context)
{
	return 0;
}
#endif
