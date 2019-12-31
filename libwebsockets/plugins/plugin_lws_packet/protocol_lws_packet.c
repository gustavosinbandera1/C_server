/*
 * ws protocol handler plugin for "POST demo"
 *
 * Copyright (C) 2010-2016 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The person who associated a work with this deed has dedicated
 * the work to the public domain by waiving all of his or her rights
 * to the work worldwide under copyright law, including all related
 * and neighboring rights, to the extent allowed by law. You can copy,
 * modify, distribute and perform the work, even for commercial purposes,
 * all without asking permission.
 *
 * These test plugins are intended to be adapted for use in your code, which
 * may be proprietary.  So unlike the library itself, they are licensed
 * Public Domain.
 */

#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <signal.h>

 struct per_session_data__lws_packet {
	struct lws_spa *spa;
	char result[LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE];
	char filename[64];
	long file_length;
	int result_len;
	long justHeader; //a flag we'll use know if we have sent a header or not...
}; 

static const char * const param_names[] = {
	"text",
	"send",
	"file",
	"upload",
};

enum enum_param_names {
	EPN_TEXT,
	EPN_SEND,
	EPN_FILE,
	EPN_UPLOAD,
};


void CreateHeaders(struct lws *wsi,void *user,char* jsonText) {
	struct per_session_data__lws_packet *pss = (struct per_session_data__lws_packet *)user;
	unsigned char *buffer;
	unsigned char *p, *start, *end;
	int n;
		p = (unsigned char *)pss->result + LWS_PRE;
		end = p + sizeof(pss->result) - LWS_PRE - 1;
		p += sprintf((char *)p,jsonText);
		pss->result_len = lws_ptr_diff(p, pss->result + LWS_PRE);

		n = LWS_PRE + 1024;
		buffer = malloc(n);
		p = buffer + LWS_PRE;
		start = p;
		end = p + n - LWS_PRE - 1;

		if (lws_add_http_header_status(wsi, HTTP_STATUS_OK, &p, end))
			goto bail;

		if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE,
				(unsigned char *)"text/html", 9, &p, end))
			goto bail;
		if (lws_add_http_header_content_length(wsi, pss->result_len, &p, end))
			goto bail;
		if (lws_finalize_http_header(wsi, &p, end))
			goto bail;

		n = lws_write(wsi, start, p - start, LWS_WRITE_HTTP_HEADERS);
		if (n < 0)
			goto bail;
		free(buffer);
return;
bail:
	free(buffer);
}


static int
callback_lws_packet(struct lws *wsi, enum lws_callback_reasons reason,
		   void *user, void *in, size_t len)
{
 	struct per_session_data__lws_packet *pss =
			(struct per_session_data__lws_packet *)user;
	unsigned char *p, *start, *end;
	
	int n; 
	switch (reason) {

		case LWS_CALLBACK_HTTP_BODY:
			fprintf(stdout,"CALLBACK HTTP BODY\n");
			fprintf(stderr,"el cuerpo es: %s\n",(char*)in);
			pss->justHeader = 0; //flag to toggle header...
		break;


		//to use with post request
		case LWS_CALLBACK_HTTP_BODY_COMPLETION:
			fprintf(stderr, "inside BODY_COMPLETION\n");
			lws_callback_on_writable(wsi);
		break;

		case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
          printf("callback_http LWS_CALLBACK_EVENT_WAIT_CANCELLED\n");
          //Once the other thread has handled the JSON object and created a new one it will call lws_cancel_service which will fire this event.
          //We will be lazy and get a callback on all everything...
          lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
        break;
//to use with http request
		case LWS_CALLBACK_HTTP_WRITEABLE:
		fprintf(stderr,"Vamos a responder");
		char *universal_response = "hola gustavo desde el server lws_write";
		//lws_write(wsi, universal_response,strlen(universal_response), LWS_WRITE_HTTP); 
		//return -1;
		 if (pss->justHeader==0) {
                  //we first send the headers...
                  CreateHeaders(wsi,user,"{ \"id\":10 }"); //hard code a responce JSON object
                  pss->justHeader=1;
                  //we need a callback so we can send the body.
                  lws_callback_on_writable(wsi);
                  return 0;
                }
                else {  
                  //we now send the body.
		  printf("LWS_CALLBACK_HTTP_WRITEABLE: sending %d\n",pss->result_len);
		  n = lws_write(wsi, (unsigned char *)pss->result + LWS_PRE,pss->result_len, LWS_WRITE_HTTP);
		  if (n < 0) return 1;
                }
                goto try_to_reuse;
		break;

		case LWS_CALLBACK_HTTP:
			fprintf(stderr, "CALLBACK HTTP---------------\n");
			char *requested_uri = (char *) in;
			printf("requested URI: %s\n", requested_uri);
			char *universal_response1 = "Hello, World!";
			if (strcmp(requested_uri, "/") == 0) {	
			}
		break;

			//< when the websocket session ends 
	    case LWS_CALLBACK_CLOSED_HTTP:
			fprintf(stderr, "CALBACK CLOSED HTTP---------------\n");
	    break;

		case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
			// called when our wsi user_space is going to be destroyed 
			fprintf(stderr, "DROP PROTOCOL---------------\n");

		break;

		default:
		break;
	}

	return 0;

	try_to_reuse:
		if (lws_http_transaction_completed(wsi))
		return -1;

	return 0;

}


#define LWS_PLUGIN_PROTOCOL_LWS_PACKET \
	{ \
		"protocol-lws-packet", \
		callback_lws_packet, \
		sizeof(struct per_session_data__lws_packet), \
		1024, \
		0, NULL, 0 \
	}

#if !defined (LWS_PLUGIN_STATIC)

static const struct lws_protocols protocols[] = {
	LWS_PLUGIN_PROTOCOL_LWS_PACKET
};

LWS_EXTERN LWS_VISIBLE int
init_protocol_lws_packet(struct lws_context *context,
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
destroy_protocol_lws_packet(struct lws_context *context)
{
	return 0;
}

#endif
