
#ifndef Datum_DEFINED
#define Datum_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct
{
	char *data;
	size_t size;
} Datum;

#define DATUM_STACKALLOCATED(name, string) Datum _##name; Datum *name = &_##name; name->data = (char *)string; name->size = strlen(string);

Datum *Datum_poolNew(void);

Datum *Datum_new(void);
Datum *Datum_newWithCString_(const char *s);
Datum *Datum_clone(Datum *d);

void Datum_free(Datum *self);
void Datum_setData_size_(Datum *self, const char *data, size_t size);
void Datum_setCString_(Datum *self, const char *s);
void Datum_setSize_(Datum *self, size_t size);
size_t Datum_size(Datum *self);
void Datum_nullTerminate(Datum *self);

const char *Datum_data(Datum *self);
void Datum_show(Datum *self);

void Datum_copy_(Datum *self, Datum *other);
void Datum_copy_and_(Datum *self, Datum *other, Datum *other2);

void Datum_selectPathComponent(Datum *self);
void Datum_selectLastComponent(Datum *self);

void Datum_append_(Datum *self, Datum *other);
void Datum_appendBytes_size_(Datum *self, const unsigned char *bytes, size_t size);
void Datum_appendQuoted_(Datum *self, Datum *other);
void Datum_appendCString_(Datum *self, const char *s);
void Datum_appendLong_(Datum *self, long n);

int Datum_equals_(Datum *self, Datum *other);
int Datum_equalsCString_(Datum *self, const char *s);

unsigned int Datum_hash1(Datum *self);
unsigned int Datum_hash2(Datum *self);

int Datum_beginsWith_(Datum *self, Datum *other);
int Datum_beginsWithCString_(Datum *self, const char *s);

int Datum_isBeginningOfCString_(Datum *self, const char *s);

void Datum_clear(Datum *self);
int Datum_isEmpty(Datum *self);

void Datum_makeIdOfLength_(Datum *self, int length);
void Datum_makePidOfLength_(Datum *self, int length);
void Datum_makePid32(Datum *self);
void Datum_makePid64(Datum *self);
void Datum_fromLong_(Datum *self, long n);
int Datum_asInt(Datum *self);
long Datum_asLong(Datum *self);

int Datum_sepOnChars_with_(Datum *self, const char *s, Datum *other);
int Datum_containsChar_(Datum *self, char c);
int Datum_charAt_(Datum *self, unsigned int index); // returns -1 for invalid indexes
int Datum_at_put_(Datum *self, unsigned int index, char c); // returns -1 for invalid indexes

void Datum_clipLastPathComponent(Datum *self);

int Datum_from_beforeChar_into_(Datum *self, unsigned int start, char c, Datum *other);
//int Datum_indexOfChar_(Datum *self, char c);

#ifdef __cplusplus
}
#endif
#endif
