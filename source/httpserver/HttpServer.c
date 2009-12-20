#include "HttpServer.h"
#include "Log.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
//#include <sys/queue.h>
#include <stdlib.h>
#include <math.h>
//#include <limits.h>
//#include <signal.h>
#include <unistd.h>
#include "Pool.h"
#include "PNode.h"

void *CHash_atString_(CHash *self, const char *s)
{
	DATUM_STACKALLOCATED(k, s);
	return CHash_at_(self, k);
}

#define HTTP_SERVERERROR 500
#define HTTP_SERVERERROR_MESSAGE "Internal Server Error"
#define HTTP_OK_MESSAGE	"OK"

HttpServer *HttpServer_new(void)
{
	HttpServer *self = calloc(1, sizeof(HttpServer));
	self->httpRequest = HttpRequest_new();
	self->httpResponse = HttpResponse_new();
	self->host = Datum_new();
	return self;
}

void HttpServer_free(HttpServer *self)
{
	HttpRequest_free(self->httpRequest);
	HttpResponse_free(self->httpResponse);
	Datum_free(self->host);
	if (self->httpd) evhttp_free(self->httpd);
	free(self);
}

HttpRequest *HttpServer_request(HttpServer *self)
{
	return self->httpRequest;
}

HttpResponse *HttpServer_response(HttpServer *self)
{
	return self->httpResponse;
}

void HttpServer_setHost_(HttpServer *self, char *host)
{
	Datum_setCString_(self->host, host);
}

Datum *HttpServer_host(HttpServer *self)
{
	return self->host;
}

void HttpServer_setPort_(HttpServer *self, int port)
{
	self->port = port;
}

int HttpServer_port(HttpServer *self)
{
	return self->port;
}

void HttpServer_setDelegate_(HttpServer *self, void *d)
{
	self->delegate = d;
}

void *HttpServer_delegate(HttpServer *self)
{
	return self->delegate;
}

void HttpServer_setRequestCallback_(HttpServer *self, HttpServerRequestCallback *f)
{
	self->requestCallback = f;
}

void HttpServer_handleRequest(struct evhttp_request *req, void *arg)  
{  
	HttpServer *self = (HttpServer *)arg;
	const char *uri = evhttp_request_uri(req);
	struct evbuffer *buf = evbuffer_new();
	
	self->request = req;
	self->httpRequest->request = req;
	self->httpResponse->request = req;
	
	HttpRequest_readPostData(self->httpRequest);
	HttpResponse_clear(self->httpResponse);
			
	if (strcmp(uri, "/favicon.ico") == 0)
	{
		evhttp_send_reply(self->request, HTTP_OK, HTTP_OK_MESSAGE, buf);
	}
	else
	{		
		HttpRequest_parseUri_(self->httpRequest, uri);
		self->requestCallback(self->delegate);

		evbuffer_add_printf(buf, "%s", Datum_data(HttpResponse_content(self->httpResponse)));

		if (HttpResponse_statusCode(self->httpResponse) == 200)
		{
			evhttp_send_reply(self->request, HTTP_OK, HTTP_OK_MESSAGE, buf);
		}
		else
		{
			evhttp_send_reply(self->request, HTTP_SERVERERROR, HTTP_SERVERERROR_MESSAGE, buf);		
		}
	}
	
	evhttp_send_reply_end(self->request);
	evbuffer_free(buf);
	Datum_poolFreeRefs();
	PNode_poolFreeRefs();
}

void HttpServer_run(HttpServer *self)
{
	if(self->shutdown) return;
	self->shutdown = 0;
	
	event_init();

	self->httpd = evhttp_start(Datum_data(self->host), self->port);
	 
	if (!self->httpd)
	{
		Log_Printf_("evhttp_start failed - is another copy running on port %i?\n", self->port);
		exit(-1);
	}
	
	evhttp_set_timeout(self->httpd, 180);
	evhttp_set_gencb(self->httpd, HttpServer_handleRequest, self);  
	
	while (!self->shutdown)
	{
		event_loop(EVLOOP_ONCE);
	}
}

void HttpServer_shutdown(HttpServer *self)
{
	self->shutdown = 1;
	event_loopbreak();
}

