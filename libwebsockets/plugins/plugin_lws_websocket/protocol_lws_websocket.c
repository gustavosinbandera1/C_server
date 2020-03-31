
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
#include "functions.h"

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
	int packetNumber;

	/*  const char* redisHost = getenv("REDIS_HOST");
    const char* redisPort = getenv("REDIS_PORT");
	const char* comlinkHost = getenv("COMLINK_HOST"); 
	*/
	const char* redisHost = "localhost";
    const char* redisPort = "6379";
	const char* comlinkHost = "http://localhost:2579/api/device/report-data";



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
		
		tokens = str_split((char*)in, ',');
	    packetNumber = atoi(*tokens);
		fprintf(stderr,"el paquete numero>>>>>>>>>>>>>>>>>%d", packetNumber);
	
		if(isCRCValid(packetNumber,bufferData) == true) {
			fprintf(stderr,"rigth packetes: ");
			//load ack on socket struct to send back to client
			char* ack = getACK(packetNumber);
			memcpy((char *)vhd->amsg.payload + LWS_PRE, ack, strlen(ack));
			vhd->amsg.len = strlen(ack);
			//get key to store data inside redis with controller code
			
			key = *(tokens+1); //device code
			key = concat(key, "_PACKET");
			
			fprintf(stderr,"the key is: %s\n",key);
			char* command = getRedisCommand(redisHost, redisPort, key, bufferData);
			fprintf(stderr, "the command to execute redis is : %s\n", command);
			fprintf(stderr, (char*)system(command));

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
			fprintf(stderr, (char*)system(command)); 
			
			//release allocated memory
			if(command) free(command);
			if(ack) free(ack);

		} else {
			fprintf(stderr,"bad packet");
			//load nack on socket struct to send back to client
			char* nack = getNck(packetNumber);
			memcpy((char *)vhd->amsg.payload + LWS_PRE, nack, strlen(nack));
			vhd->amsg.len = strlen(nack);
			if(nack) free(nack);
		}
	
		vhd->current++;
		free(bufferData);
		
		/*
		 * let everybody know we want to write something on them
		 * as soon as they are ready
		 */
		//write to socket when possible, vhd struct with payload message will be send to client 
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
