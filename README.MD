
- [COMLINK VERSION 2 IMPLEMENTATION IN C](#comlink-version-2-implementation-in-c)
	- [Libwebsockets](#libwebsockets)
		- [Dockerfile](#dockerfile)
			- [Code snippet 1](#code-snippet-1)
			- [Code snippet 2](#code-snippet-2)
			- [Code snippet 3.](#code-snippet-3)
			- [lwsws_comlink_v2_conf](#lwswscomlinkv2conf)
			- [lwsws_comlink_v2_host](#lwswscomlinkv2host)
			- [Code snippet 4](#code-snippet-4)
			- [Building Dockerfile](#building-dockerfile)
		- [How to work with plugins](#how-to-work-with-plugins)
			- [compiling plugin-standalone](#compiling-plugin-standalone)
				- [How the plugin standalone example looks](#how-the-plugin-standalone-example-looks)
				- [Main structure of the plugin](#main-structure-of-the-plugin)
			- [Callback prototype](#callback-prototype)
				- [struct lws *wsi](#struct-lws-wsi)
				- [enum lws_callback_reasons reason](#enum-lwscallbackreasons-reason)
				- [void* in](#void-in)
				- [state machine](#state-machine)
				- [section writeable Only send data when socket writeable](#section-writeable-only-send-data-when-socket-writeable)
				- [section oneper Only one lws_write per WRITEABLE callback](#section-oneper-only-one-lwswrite-per-writeable-callback)


# COMLINK VERSION 2 IMPLEMENTATION IN C

This new server implementation aims to manage the communication of the controller and receive its packages and make them available to other entities related to the acquisition of information.
The main objective is to eliminate the incompatibilities that arise when trying to stablish a reliable communication between the controller and the current server written in nodejs.

- This guide tries to show you how to deploy the server and how to create plugins to work with.
- This documment is going to avoid library build steps, because all needed things to work with this server, they are wrapper in a Docker file step by step,
- This guide is going to explain step by step the process inside Docker file to have server running in proper way.




## Libwebsockets

The main library to work in **comlink V2** is **lwsws** library, that is simple to use pure C library providing client and server for http/1, http/2, websockets and other protocols in a security-minded, lightweight, configurable, scalable and flexible way. It's easy to build and cross-build via cmake and is suitable for tasks from embedded RTOS through mass cloud serving.

This is a highly maintained library to date, with lots of information and more than 50 practical examples showing how to use the library.

If anyone want to get more information about the library used here in this challenge, can check this [link](https://github.com/warmcat/libwebsockets/blob/master/READMEs/README.lwsws.md).

### Dockerfile

The Docker file is in order to deploy in easy way the server and do it compactible with the actual ECS cluster; This one is stored in the root of the project. 

We are going to try to explain how the docker process is, and how to run it.
Having told that, let me get start.

#### Code snippet 1

```
COPY libwebsockets /usr/local/src/libwebsockets 

WORKDIR /usr/local/src/libwebsockets
RUN rm -r build
RUN mkdir build
WORKDIR /usr/local/src/libwebsockets/build

RUN cmake .. -DLWS_WITH_LWSWS=1 -DCMAKE_BUILD_TYPE=DEBUG
RUN make && make install
```

The **code snipped 1** is the main important thing to have built the server, first copying the complete folder **libwebsockets**
inside container, then removing a possible build folder to avoid issues and after that make and install the server.
but the process continue we need to configure the server to be executed inside container, lets go... 
We can see the flag **-DLWS_WITH_LWSWS=1** this enable libuv package and plugin support automatically.

#### Code snippet 2

```
ENV LD_LIBRARY_PATH /usr/local/lib:${LD_LIBRARY_PATH}
RUN ldconfig -v 
```
The code snippet 2 is in order to set up the library environment variable, $LD_LIBRARY_PATH is consulted at time of execution, to provide a list of additional directories in which to search for dynamically linkable libraries. 



#### Code snippet 3.
```
RUN mkdir /etc/lwsws
RUN mkdir /etc/lwsws/conf.d             # to store one file for each Host, we can define many different host here 
RUN mkdir /var/log/lwsws                # to store logs from server
RUN mkdir /usr/local/lib/systemd        # to run server as a service inside container


# Copy the some configs
ADD ./lwsws_conf /etc/lwsws/conf
ADD ./lwsws_my_vhost /etc/lwsws/conf.d/test-server
ADD ./lwsws_systemd.service /usr/local/lib/systemd/lwsws.service
ADD ./lwsws_logrotate /etc/logrotate.d/lwsws
ADD ./check.html /usr/local/share/libwebsockets-test-server/check.html

```
The code snippet 3 just is creating a needed folders to configure the server and copy some configuartion files.
Inside root folder we can four files: 

1. lwsws_comlink_v2_conf
2. lwsws_comlink_v2_host
3. lwsws_comlink_v2_logrotate
4. lwsws_comlink_v2_systemd.service

#### lwsws_comlink_v2_conf

There is a single file intended for global settings

```
{
  "global": {
   "username": "AMS-TEAM",
	 "groupname": "AMS-TEAM",
   "uid": "33",
   "gid": "33",
   "interface": "eth0",
   "count-threads": "1",
   "server-string": "lwsws",
   "ws-pingpong-secs": "200",
   "init-ssl": "yes"
   }
}

```

#### lwsws_comlink_v2_host
and a config directory intended to take one file per vhost, you can found more information about the topic [here](https://github.com/warmcat/libwebsockets/blob/master/READMEs/README.lwsws.md)
**/etc/lwsws/conf.d/comlink_v2**

```json
{
 "vhosts": [ {
     "name": "localhost",
     "port": "7681",
# by default, bind to all interfaces, but you can restrict it
#     "interface": "lo",
#     "host-ssl-key":  "/etc/pki/tls/private/libwebsockets.org.key",
#     "host-ssl-cert": "/etc/pki/tls/certs/libwebsockets.org.crt",
#     "host-ssl-ca":   "/etc/pki/tls/certs/libwebsockets.org.cer",
     "access-log": "/var/log/lwsws/test-access-log",
     "sts": "on",
     "mounts": [{
       "mountpoint": "/",
       "origin": "file://_lws_ddir_/libwebsockets-test-server",
       "default": "test.html",
       "cache-max-age": "60",
       "cache-reuse": "1",
       "cache-revalidate": "1",
       "cache-intermediaries": "0"
       }, {
        "mountpoint": "/server-status",
        "origin": "file://_lws_ddir_/libwebsockets-test-server/server-status",
        "default": "server-status.html"
        }, {
        "mountpoint": "/testcgi",
        #"origin": "cgi://_lws_ddir_/libwebsockets-test-server/lws-cgi-test.sh"
         "origin": "cgi://_lws_ddir_/libwebsockets-test-server/lws-cgi-test.sh"

       }, {
        "mountpoint": "/formtest",
        "origin": "callback://protocol-post-demo"
       },
       {
           "mountpoint": "/formtest2",
           "origin": "callback://protocol-lws-packet"
        }
		],
		

     # which protocols are enabled for this vhost, and optional
     # vhost-specific config options for the protocol
     #
        "ws-protocols": [{
       "lws-meta": {
         "status": "ok"
       },
       "dumb-increment-protocol": {
         "status": "ok"
       },
       "lws-mirror-protocol": {
         "status": "ok"
       },
       "lws-status": {
         "status": "ok"
       },

       "protocol-post-demo": {
         "status": "ok"
       },
        "protocol-lws-packet":{
          "status":"ok"
        },
       "lws-server-status": {
         "status": "ok",
         "update-ms": "5000"
       },
         "example-standalone-protocol": {
          "status": "ok"
        },
        "lws-websocket":{
           "status": "ok"
        }
     }]
    }
  ]
}

```


#### Code snippet 4
```
WORKDIR /usr/local/src/libwebsockets/plugin-standalone
RUN mkdir build
WORKDIR /usr/local/src/libwebsockets/plugin-standalone/build
RUN cmake ..
RUN make
RUN make install
```
The first line in the block of code is in order to get the path of the code plugin, the next steps are intended to build the plugin; If you want to create the custom plugin, when plugin ready, you'll just need to copy this command and edit it, thenpaste inside dockerfile. When the plugin is built, we have created a new dinamic shared library and stored in lib path as is describe [here](#code-snippet-2)

#### Building Dockerfile 

To have Comlink version 2 running we need first to build and run the image created based on this docker documment.

we need to get the root path of the comlink v2 server and execute the next command.

```
sudo docker build -t comlink_v2 .
```
**Fihure 1**
![alt text](doc/build1.png)

After build process finished, we are able to see something very close to the above (Fig 1), it is called **Docker image**; And In this point the next step is try to run the conatiner with the newest image.

```
sudo docker run -p 2579:2579 -it comlink_v2

```

Is highest probably you want to access to the container by ssh, if the cases is this, you can execute the next command in order to reach the goal.

```
docker exec -it container_id /bin/bash 
in this case 
docker exec -it a993b2096be7 /bin/bash
```

And with this latest command we are inside comlink_v2 machine.
Having done the minimalistic steps, let my try to explain how to build and create new plugins to work with the new server. It should be noted that its no necessary to modify the server code to add new features, all that you'll need is understand how the plugins system works and get start very quickly with this awesome technology to work with  servers in C.




### How to work with plugins

To create any plugin we need to rewrite plugin-standalone that is an example, and is hosted inside **comlink_v2/libwebsockets/plugin-standalone**
we can inside plugin-standalone folder create a build directory and inside this latest one execute the next commands to build and create the shared library, the same as linux shared libraries.
```
cmake ..
make && sudo make install 
```
But the good news are we dont will need to do any of the previus steps by ourself, because the dockerfile has a section to write the needed command , and you'll just need to copy and paste the required command and docker can do the dirty work for you, even if you are working in local machine, this server can be deployed in easy way in a very short time maybe 10 minutes without having any issue.
How to build the pluging using Dockerfile is described [here](#code-snippet-4)


#### compiling plugin-standalone




##### How the plugin standalone example looks

This is the very basic plugin to ilustrate how to create customs plugins. 

```cpp
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>

#include <string.h>

struct per_vhost_data__dumb_increment {
	uv_timer_t timeout_watcher;
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;
};

struct per_session_data__dumb_increment {
	int number;
};

static void
uv_timeout_cb_dumb_increment(uv_timer_t *w
#if UV_VERSION_MAJOR == 0
		, int status
#endif
)
{
	struct per_vhost_data__dumb_increment *vhd = lws_container_of(w,
			struct per_vhost_data__dumb_increment, timeout_watcher);
	lws_callback_on_writable_all_protocol_vhost(vhd->vhost, vhd->protocol);
}

static int
callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct per_session_data__dumb_increment *pss =
			(struct per_session_data__dumb_increment *)user;
	struct per_vhost_data__dumb_increment *vhd =
			(struct per_vhost_data__dumb_increment *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	unsigned char buf[LWS_PRE + 512];
	unsigned char *p = &buf[LWS_PRE];
	int n, m;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct per_vhost_data__dumb_increment));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);
		uv_timer_init(lws_uv_getloop(vhd->context, 0),
			      &vhd->timeout_watcher);
		uv_timer_start(&vhd->timeout_watcher,
			       uv_timeout_cb_dumb_increment, 50, 50);
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		if (!vhd)
			break;
		uv_timer_stop(&vhd->timeout_watcher);
		break;

	case LWS_CALLBACK_ESTABLISHED:
		pss->number = 0;
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		n = sprintf((char *)p, "%d", pss->number++);
		m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		if (len < 6)
			break;
		if (strcmp((const char *)in, "reset\n") == 0)
			pss->number = 0;
		if (strcmp((const char *)in, "closeme\n") == 0) {
			lwsl_notice("dumb_inc: closing as requested\n");
			lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,
					 (unsigned char *)"seeya", 5);
			return -1;
		}
		break;

	default:
		break;
	}

	return 0;
}

static const struct lws_protocols protocols[] = {
	{
		"example-standalone-protocol",
		callback_dumb_increment,
		sizeof(struct per_session_data__dumb_increment),
		10, /* rx buf size must be >= permessage-deflate rx size */
	},
};

LWS_VISIBLE int
init_protocol_example_standalone(struct lws_context *context,
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

LWS_VISIBLE int
destroy_protocol_example_standalone(struct lws_context *context)
{
	return 0;
}
```


In the first look could be hard try to understand the code, but dont worry, really its not hard, just trainning its enough to be more confident.

##### Main structure of the plugin

We can see there is a function called **callback_dumb_increment**, this is the most important function, in all pluging are you going to create.

That funtion is a callback funciton and always you'll need to create the same prototype to be used with new plugins, let me talk a little bit about the inner structure.

#### Callback prototype
You can create your own callback name function, but the parameters need to be the same always.
There are more information about callback functions [here](https://libwebsockets.org/lws-api-doc-master/html/group__callback-when-writeable.html).

User protocol callback

The protocol callback is the primary way lws interacts with user code. For one of a list of a few dozen reasons the callback gets called at some event to be handled.

All of the events can be ignored, returning 0 is taken as "OK" and returning nonzero in most cases indicates that the connection should be closed. 

```Java
callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len) { }
```

##### struct lws *wsi

We can use this object to get information about the client **context, protocol, vhost**, for instance when prtocol Init.

```Ruby
case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct per_vhost_data__dumb_increment));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);
		uv_timer_init(lws_uv_getloop(vhd->context, 0),
			      &vhd->timeout_watcher);
		uv_timer_start(&vhd->timeout_watcher,
			       uv_timeout_cb_dumb_increment, 50, 50);
		break;

```

#####  enum lws_callback_reasons reason
With this oaram we can handle the state machine to do whatever we want.
For more information about ```reason``` definition, you can clieck [here](https://libwebsockets.org/lws-api-doc-master/html/group__usercb.html).
You can see how to use callbacks and state machine in this very basic useful [example](https://github.com/warmcat/libwebsockets/blob/master/plugins/protocol_client_loopback_test.c)

##### void* in

This void* pointer is used for some calback functions, and can gave you information, i.e from POST request we can capture the body with this pointer.

```Ruby
#include <include/libwebsockets/lws-callbacks.h>

typedef lws_callback_function() - User server actions

Parameters
    wsi:-->		Opaque websocket instance pointer
    reason:-->	The reason for the call
    user:-->	Pointer to per-session user data allocated by library
    in:-->		Pointer used for some callback reasons
    len:-->		Length set for some callback reasons
	'
     This callback is the way the user controls what is served.  All the
     protocol detail is hidden and handled by the library.

     For each connection / session there is user data allocated that is
     pointed to by "user".  You set the size of this user data area when
     the library is initialized with lws_create_server.'
	

```

##### state machine
That is the minimalistic state machine, we could capture more different [events](https://libwebsockets.org/lws-api-doc-v2.2-stable/html/group__usercb.html), This state machine is running in a loop.

```Ruby
switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		break;

	case LWS_CALLBACK_ESTABLISHED:
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		break;

	case LWS_CALLBACK_RECEIVE:
		break;

	default:
		break;
	}
```



##### section writeable Only send data when socket writeable

You should only send data on a websocket connection from the user callback LWS_CALLBACK_SERVER_WRITEABLE (or LWS_CALLBACK_CLIENT_WRITEABLE for clients).

If you want to send something, do NOT just send it but request a callback when the socket is writeable using
```
    lws_callback_on_writable(wsi) for a specific wsi, or

    lws_callback_on_writable_all_protocol(protocol) for all connections using that protocol to get a callback when next writeable.
```

Usually you will get called back immediately next time around the service loop, but if your peer is slow or temporarily inactive the callback will be delayed accordingly. Generating what to write and sending it should be done in the ...WRITEABLE callback.


##### section oneper Only one lws_write per WRITEABLE callback

From v2.5, lws strictly enforces only one lws_write() per WRITEABLE callback.

You will receive a message about "Illegal back-to-back write of ... detected" if there is a second lws_write() before returning to the event loop.

Note that if you are handling your own http responses, writing the headers needs to be done with a separate lws_write() from writing any payload. That means after writing the headers you must call lws_callback_on_writable(wsi) and send any payload from the writable callback.
**We need to be sure never ever try to use lws_write more than one time in WRITEABLE callback. instead of, of you want to send more data , you can use lws_callback_on_writable(wsi)**, this lates could be used in whatever part of the state machine.

**Right**
```cpp
	case LWS_CALLBACK_SERVER_WRITEABLE:
		n = sprintf((char *)p, "%d", pss->number++);
		m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

```
**Right**
```cpp
	case LWS_CALLBACK_SERVER_WRITEABLE:
		n = sprintf((char *)p, "%d", pss->number++);
		m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
		lws_callback_on_writable(wsi); //wsi is a structure with data, this information could be sent when the loop can be ready, we dont need to worry about it
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

```


**wrong**
```cpp
	case LWS_CALLBACK_SERVER_WRITEABLE:
		n = sprintf((char *)p, "%d", pss->number++);
		m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
		lws_write(wsi, p, n, LWS_WRITE_TEXT); //even if params are different its not possible to use
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

```

For instance in this case we have a custom function to create header, that is why we'll need to use **lws_write** to send the headers, inside function, but if we used that funciton to write information to client again, we cannot use anymore  inside the loop, so that we need to use **lws*_callback_on_writable** and gave the control to the loop to send the message to the client when resource available.

**Create Headers function**
```cpp
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

```

**Right** We are just using one time **lws_write**
```cpp
case LWS_CALLBACK_HTTP_WRITEABLE:
		fprintf(stderr,"Vamos a responder");
		char *universal_response = "hola gustavo desde el serverlws_write";
		//lws_write(wsi, universal_response,strlen(universal_response), LWS_WRITE_HTTP); 
		//return -1;
		 if (pss->justHeader==0) {
                  //we first send the headers...
                  CreateHeaders(wsi,user,"{ \"id\":1 }"); //hard code a responce JSON object
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
```







