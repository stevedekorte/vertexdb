#include "HttpRequest.h"
#include "Log.h"
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
#include "HttpServer.h"


void Datum_decodeUri(Datum *self)
{
	char *k = evhttp_decode_uri(Datum_data(self));
	Datum_setCString_(self, k);
	free(k);
}

void Datum_encodeUri(Datum *self)
{
	char *k = evhttp_encode_uri(Datum_data(self));
	Datum_setCString_(self, k);
	free(k);
}

Datum *Datum_asUriEncoded(Datum *self)
{
	char *k = evhttp_encode_uri(Datum_data(self));
	Datum *d = Datum_poolNewWithCString_(k);
	free(k);
	return d;
}

HttpRequest *HttpRequest_new(void)
{
	HttpRequest *self = calloc(1, sizeof(HttpRequest));
	self->post = Datum_new();
	self->uriPath = Datum_new(); 
	
	self->query = CHash_new();	
	CHash_setEqualFunc_(self->query, (CHashEqualFunc *)Datum_equals_);
	CHash_setHash1Func_(self->query, (CHashHashFunc *)Datum_hash1);
	CHash_setHash2Func_(self->query, (CHashHashFunc *)Datum_hash2);

	self->emptyDatum = Datum_new();

	return self;
}

void HttpRequest_free(HttpRequest *self)
{
	Datum_free(self->post);
	Datum_free(self->uriPath);
	CHash_free(self->query);
	Datum_free(self->emptyDatum);
	free(self);
}

void HttpRequest_parseUri_(HttpRequest *self, const char *uri)
{
	int index;
	Datum *uriDatum = Datum_poolNew();
	Datum_setCString_(uriDatum, uri);
	
	//if (self->debug) { Log_Printf_("request: %s\n", uri); }
	
	CHash_clear(self->query);
	
	Datum_setSize_(self->uriPath, 0);
	index = Datum_from_beforeChar_into_(uriDatum, 1, '?', self->uriPath);
	Datum_decodeUri(self->uriPath);
	
	for (;;)
	{
		Datum *key   = Datum_poolNew();
		Datum *value = Datum_poolNew();
		
		index = Datum_from_beforeChar_into_(uriDatum, index + 1, '=', key);
		Datum_decodeUri(key);
		Datum_nullTerminate(key);
		if (Datum_size(key) == 0) break;
		
		index = Datum_from_beforeChar_into_(uriDatum, index + 1, '&', value);
		Datum_decodeUri(value);
		Datum_nullTerminate(value);

		CHash_at_put_(self->query, key, value);
		if (Datum_size(value) == 0) break;
	}	
}

Datum *HttpRequest_uriPath(HttpRequest *self)
{
	return self->uriPath;
}

Datum *HttpRequest_queryValue_(HttpRequest *self, const char *key)
{
	Datum *value = CHash_atString_(self->query, key);
	return value ? value : self->emptyDatum;
}

void HttpRequest_readPostData(HttpRequest *self)  
{
	struct evbuffer *evb = self->request->input_buffer;
	Datum_setData_size_(self->post, (const char *)EVBUFFER_DATA(evb), EVBUFFER_LENGTH(evb));
}

Datum *HttpRequest_postData(HttpRequest *self)  
{
	return self->post;
}

