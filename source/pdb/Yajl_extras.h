
#ifndef YAJL_EXTRAS_DEFINED
#define YAJL_EXTRAS_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include "Datum.h"
#include <yajl/yajl_gen.h>

void yajl_gen_cstring(yajl_gen self, const char *s);
void yajl_gen_datum(yajl_gen self, Datum *d);
void Datum_appendYajl_(Datum *self, yajl_gen y);
void Datum_setYajl_(Datum *self, yajl_gen y);

#ifdef __cplusplus
}
#endif
#endif
