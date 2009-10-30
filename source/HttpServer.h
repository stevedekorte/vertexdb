#ifndef HttpServer_DEFINED
#define HttpServer_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "Date.h"

typedef void  (HttpServerRequestCallback)(void *);

typedef struct
{
	void *delegate;
	HttpServerRequestCallback *requestCallback;
} HttpServer;

HttpServer *HttpServer_new(void);
void HttpServer_free(HttpServer *self);

void HttpServer_setDelegate_(HttpServer *self, void *d);
void HttpServer_setRequestCallback_(HttpServer *self, HttpServerRequestCallback *f);


// UNFISHED - THIS WILL ABSTRACT OVER libevent SO WE CAN SWAP IT WITH libev

// read query
// read post
// write to buffer
// set response status
// send response

#ifdef __cplusplus
}
#endif
#endif
