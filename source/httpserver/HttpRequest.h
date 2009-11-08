#ifndef HttpRequest_DEFINED
#define HttpRequest_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#include <err.h>
#include <event.h>
#include <evhttp.h>
#include "CHash.h"
#include "Datum.h"

#define HTTP_SERVERERROR 500
#define HTTP_SERVERERROR_MESSAGE "Internal Server Error"
#define HTTP_OK_MESSAGE	"OK"

typedef void  (HttpRequestRequestCallback)(void *);

typedef struct
{
	struct evhttp_request *request; // doesn't own this
	Datum *uriPath;
	Datum *post;
	CHash *query;
	Datum *error;
	Datum *emptyDatum;
	HttpRequestRequestCallback *requestCallback;
} HttpRequest;

HttpRequest *HttpRequest_new(void);
void HttpRequest_free(HttpRequest *self);

Datum *HttpRequest_uriPath(HttpRequest *self);
void HttpRequest_parseUri_(HttpRequest *self, const char *uri);

void HttpRequest_setDelegate_(HttpRequest *self, void *d);
void HttpRequest_setRequestCallback_(HttpRequest *self, HttpRequestRequestCallback *f);

Datum *HttpRequest_queryValue_(HttpRequest *self, const char *key);

void HttpRequest_readPostData(HttpRequest *self); 
Datum *HttpRequest_postData(HttpRequest *self);

#ifdef __cplusplus
}
#endif
#endif
