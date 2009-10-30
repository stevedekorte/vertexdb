#include "HttpServer.h"
#include "Log.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

HttpServer *HttpServer_new(void)
{
	HttpServer *self = calloc(1, sizeof(HttpServer));

	return self;
}

void HttpServer_free(HttpServer *self)
{
	free(self);
}

void HttpServer_setDelegate_(HttpServer *self, void *d)
{
	self->delegate = d;
}

void HttpServer_setRequestCallback_(HttpServer *self, HttpServerRequestCallback *f)
{
	self->requestCallback = f;
}


void HttpServer_readAnyPostData(HttpServer *self)  
{
/*
	struct evbuffer *evb = self->request->input_buffer;
	Datum_setData_size_(self->post, (const char *)EVBUFFER_DATA(evb), EVBUFFER_LENGTH(evb));
*/
}

void HttpServer_handleRequest(HttpServer *self)  
{  
/*
	HttpServer *self = arg;
	const char *uri = evhttp_request_uri(req);
	int result;
	struct evbuffer *buf = evbuffer_new();
	
	self->request = req;
	HttpServer_setupYajl(self); 
	
	HttpServer_readAnyPostData(self);
			
	if (strcmp(uri, "/favicon.ico") == 0)
	{
		evhttp_send_reply(self->request, HTTP_OK, HTTP_OK_MESSAGE, buf);
	}
	else
	{
		HttpServer_parseUri_(self, uri);

		Datum_clear(self->result);
		self->isHtml = 0;
		result = HttpServer_process(self);

		if (result == 0)
		{
			if (Datum_size(self->result))
			{
				Datum_nullTerminate(self->result);
				evbuffer_add_printf(buf, "%s", Datum_data(self->result));
			}
			else
			{
				evbuffer_add_printf(buf, "null");
			}

			evhttp_add_header(self->request->output_headers, "Content-Type", self->isHtml ? "text/html;charset=utf-8" : "application/json;charset=utf-8");
			
			evhttp_send_reply(self->request, HTTP_OK, HTTP_OK_MESSAGE, buf);
		}
		else
		{
			evhttp_add_header(self->request->output_headers, "Content-Type", "application/json;charset=utf-8");
			if (Datum_size(self->error))
			{
				Datum_nullTerminate(self->error); 
				
				yajl_gen_clear(self->yajl);
				yajl_gen_datum(self->yajl, self->error);
				Datum_setYajl_(self->error, self->yajl);
				
				evbuffer_add_printf(buf, "%s", Datum_data(self->error));
				
				if(self->debug) 
				{
					Log_Printf_("REQUEST ERROR: %s\n", Datum_data(self->error));
				}
				
				Datum_setSize_(self->error, 0);
			}
			else
			{
				evbuffer_add_printf(buf, "\"unknown error\"");
			}
			
			evhttp_send_reply(self->request, HTTP_SERVERERROR, HTTP_SERVERERROR_MESSAGE, buf);		
		}
	}
	
	evhttp_send_reply_end(self->request);
	evbuffer_free(buf);
	
	#ifdef COMMIT_PERIODICALLY
	HttpServer_commitIfNeeded(self);
	#endif
	
	if(self->requestCount % self->requestsPerSample == 0)
	{
		Log_Printf__("STATS: %i requests, %i bytes to write\n", 
			(int)self->requestCount, (int)PDB_bytesWaitingToCommit(self->pdb));
	}

	self->requestCount ++;
	Pool_freeRefs(self->pool);
	PDB_freeNodes(self->pdb);
*/
}

void HttpServer_runEventLoop(HttpServer *self)
{
/*
	event_init();
	self->httpd = evhttp_start("127.0.0.1", self->port);
	 
	if (!self->httpd)
	{
		Log_Printf("evhttp_start failed - is another copy running on the same port?\n");
		exit(-1);
	}
	
	evhttp_set_timeout(self->httpd, 180);
	evhttp_set_gencb(self->httpd, HttpServer_requestHandler, self);  
	//Log_Printf_("libevent using: %s\n", event_get_action());
	
	while (!self->shutdown)
	{
		event_loop(EVLOOP_ONCE);
	}
*/
}
