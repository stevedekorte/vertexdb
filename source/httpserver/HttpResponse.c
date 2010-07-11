#include "HttpResponse.h"
#include "HttpServer.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

HttpResponse *HttpResponse_new(void)
{
	HttpResponse *self = calloc(1, sizeof(HttpResponse));
	self->content = Datum_new();
	self->callback = Datum_new();
	return self;
}

void HttpResponse_free(HttpResponse *self)
{
	Datum_free(self->content);
	Datum_free(self->callback);
	free(self);
}

void HttpResponse_setContentType_(HttpResponse *self, const char *v)
{
	HttpResponse_setHeader_to_(self, "Content-Type", v);
}

void HttpResponse_setHeader_to_(HttpResponse *self, const char *k, const char *v)
{
	evhttp_remove_header(self->request->output_headers, k);
	evhttp_add_header(self->request->output_headers, k, v);
}

void HttpResponse_setCookie_(HttpResponse *self, Datum *cookie)  
{
	HttpResponse_setHeader_to_(self, "Content-Type", Datum_data(cookie));
}

void HttpResponse_setCallback_(HttpResponse *self, Datum *c) {
	Datum_copy_(self->callback, c);
}

void HttpResponse_setContent_(HttpResponse *self, Datum *d)
{
	Datum_copy_(self->content, d);
}

void HttpResponse_setContentCString_(HttpResponse *self, const char *s)
{
	Datum_setCString_(self->content, s);
}

void HttpResponse_appendContent_(HttpResponse *self, Datum *d)
{
	Datum_append_(self->content, d);
}

void HttpResponse_appendContentCString_(HttpResponse *self, const char *s)
{
	Datum_appendCString_(self->content, s);
}

Datum *HttpResponse_content(HttpResponse *self)
{
	return self->content;
}

void HttpResponse_setStatusCode_(HttpResponse *self, int statusCode)
{
	self->statusCode = statusCode;
}

int HttpResponse_statusCode(HttpResponse *self)
{
	return self->statusCode;
}

void HttpResponse_clear(HttpResponse *self)
{
	self->statusCode = 200;
	//Datum_clear(self->cookie);
	Datum_clear(self->content);
}

