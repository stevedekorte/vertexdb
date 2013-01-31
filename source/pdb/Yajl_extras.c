#include "Yajl_extras.h"
#include <stdio.h>
#include <string.h>

void yajl_gen_cstring(yajl_gen self, const char *s)
{
	yajl_gen_string(self, (const unsigned char *)s, strlen(s));
}

void yajl_gen_datum(yajl_gen self, Datum *d)
{
	yajl_gen_string(self, (const unsigned char *)Datum_data(d), Datum_size(d));
}

void Datum_appendYajl_(Datum *self, yajl_gen y)
{
	const unsigned char *jsonBuffer;
	size_t jsonBufferLength;
		
	yajl_gen_get_buf(y, &jsonBuffer, &jsonBufferLength);
	
	Datum_appendBytes_size_(self, jsonBuffer, (size_t)jsonBufferLength);
	yajl_gen_clear(y);
}

void Datum_setYajl_(Datum *self, yajl_gen y)
{
	const unsigned char *jsonBuffer;
	size_t jsonBufferLength;
		
	yajl_gen_get_buf(y, &jsonBuffer, &jsonBufferLength);
	
	Datum_setData_size_(self, (char *)jsonBuffer, (size_t)jsonBufferLength);
	yajl_gen_clear(y);
}




