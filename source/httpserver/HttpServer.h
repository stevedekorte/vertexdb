#ifndef HttpServer_DEFINED
#define HttpServer_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#include <event.h>
#include <evhttp.h>
#include <err.h>
#include "HttpRequest.h"
#include "HttpResponse.h"

typedef void (HttpServerRequestCallback)(void *);
typedef void (HttpServerIdleCallback)(void *);
//typedef void  (HttpServerWriteMethod)(void *);

#define HTTP_SERVERERROR 500
#define HTTP_SERVERERROR_MESSAGE "Internal Server Error"
#define HTTP_OK_MESSAGE	"OK"

typedef struct
{
	struct evhttp *httpd;
	struct evhttp_request *request;
	
	Datum *host;
	int port;
	
	void *delegate;
	HttpServerRequestCallback *requestCallback;
	HttpServerIdleCallback *idleCallback;
	
	int shutdown;
	
	HttpRequest *httpRequest;
	HttpResponse *httpResponse;
} HttpServer;

void *CHash_atString_(CHash *self, const char *s);

HttpServer *HttpServer_new(void);
void HttpServer_free(HttpServer *self);

HttpRequest *HttpServer_request(HttpServer *self);
HttpResponse *HttpServer_response(HttpServer *self);

void HttpServer_setHost_(HttpServer *self, char *host);
Datum *HttpServer_host(HttpServer *self);

void HttpServer_setPort_(HttpServer *self, int port);
int HttpServer_port(HttpServer *self);

void HttpServer_setDelegate_(HttpServer *self, void *d);
void HttpServer_setRequestCallback_(HttpServer *self, HttpServerRequestCallback *f);
void HttpServer_setIdleCallback_(HttpServer *self, HttpServerIdleCallback *f);

void HttpServer_run(HttpServer *self);
void HttpServer_shutdown(HttpServer *self);

#ifdef __cplusplus
}
#endif
#endif
