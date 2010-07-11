#ifndef HttpResponse_DEFINED
#define HttpResponse_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "Datum.h"

typedef struct
{
	struct evhttp_request *request; // doesn't own this
	int statusCode;
	Datum *content;
	Datum *callback; // used for JSONP support
} HttpResponse;

HttpResponse *HttpResponse_new(void);
void HttpResponse_free(HttpResponse *self);

void HttpResponse_setContentType_(HttpResponse *self, const char *v);
void HttpResponse_setHeader_to_(HttpResponse *self, const char *k, const char *v);

void HttpResponse_setContent_(HttpResponse *self, Datum *d);
void HttpResponse_setContentCString_(HttpResponse *self, const char *s);
	
void HttpResponse_setCallback_(HttpResponse *self, Datum *c);

void HttpResponse_appendContent_(HttpResponse *self, Datum *d);
void HttpResponse_appendContentCString_(HttpResponse *self, const char *s);

Datum *HttpResponse_content(HttpResponse *self);

void HttpResponse_setStatusCode_(HttpResponse *self, int statusCode);
int HttpResponse_statusCode(HttpResponse *self);

void HttpResponse_setCookie_(HttpResponse *self, Datum *cookie);

void HttpResponse_clear(HttpResponse *self);

#ifdef __cplusplus
}
#endif
#endif
