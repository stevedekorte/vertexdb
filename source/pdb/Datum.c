
#include "Datum.h"
#include "Common.h"
#include "Hash_murmur.h"
#include "Hash_superfast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Datum *Datum_new(void)
{
	Datum *self = calloc(1, sizeof(Datum));
	Datum_setSize_(self, 0);
	return self;
}

Datum *Datum_newWithCString_(const char *s)
{
	Datum *self = Datum_new();
	Datum_setCString_(self, s);
	return self;
}

Datum *Datum_clone(Datum *d)
{
	Datum *self = Datum_new();
	Datum_copy_(self, d);
	return self;
}

/*
Datum *Datum_newData_size_copy_(char *data, size_t size, int copy)
{
	Datum *self = Datum_new();
	
	if (copy)
	{
		Datum_setData_size_(self, data, size);
	}
	else
	{
		self->data = data;
		self->size = size;
	}
	
	return self;
}
*/

void Datum_free(Datum *self)
{
	if (self->data) free(self->data);
	free(self);
}

void Datum_setSize_(Datum *self, size_t size)
{
	self->data = realloc(self->data, size +1);
	self->size = size;
	self->data[size] = 0x0;
}

void Datum_nullTerminate(Datum *self)
{
	self->data = realloc(self->data, self->size + 1);
	self->data[self->size] = 0x0;
}

void Datum_setData_size_(Datum *self, const char *data, size_t size)
{
	Datum_setSize_(self, size);
	memmove(self->data, data, size);
}

void Datum_setCString_(Datum *self, const char *s)
{
	Datum_setData_size_(self, s, strlen(s));
}

size_t Datum_size(Datum *self)
{
	return self->size;
}

const char *Datum_data(Datum *self)
{
	return self->data;
}

void Datum_show(Datum *self)
{
	// assume null terminated
	printf("%s", self->data); 
}

void Datum_copy_(Datum *self, Datum *other)
{
	Datum_setData_size_(self, Datum_data(other), Datum_size(other));
}

void Datum_copy_and_(Datum *self, Datum *other, Datum *other2)
{
	Datum_setData_size_(self, Datum_data(other), Datum_size(other));
	Datum_append_(self, other2);
}

void Datum_append_(Datum *self, Datum *other)
{
	size_t oldSize = self->size;
	Datum_setSize_(self, self->size + other->size);
	memcpy(self->data + oldSize, other->data, other->size);
}

void Datum_appendBytes_size_(Datum *self, const unsigned char *bytes, size_t size)
{
	size_t oldSize = self->size;
	Datum_setSize_(self, self->size + size);
	memcpy(self->data + oldSize, bytes, size);
}

void Datum_appendQuoted_(Datum *self, Datum *other)
{
	Datum_appendCString_(self, "\""); 
	Datum_append_(self, other);
	Datum_appendCString_(self, "\""); 
}

void Datum_appendCString_(Datum *self, const char *s)
{
	int len = strlen(s);
	size_t oldSize = self->size;
	Datum_setSize_(self, self->size + len);
	//strcat(self->data, s);
	memcpy(self->data + oldSize, s, len);
}

void Datum_appendLong_(Datum *self, long n)
{
	char ns[128];
	snprintf(ns, 127, "%u", (unsigned int)n);
	Datum_appendCString_(self, ns);
}

void Datum_selectPathComponent(Datum *self)
{
	char *s = strrchr(self->data, '/');
	
	if (s) 
	{ 
		s ++;
		*s = 0x0;
		self->size = s - self->data;
	}
}

void Datum_selectLastComponent(Datum *self)
{
	char *s = strrchr(self->data, '/');
	
	if (s) 
	{
		int len;
		s ++;
		len = self->size - (s - self->data);
		memmove(self->data, s, len);
		self->size = len;
		self->data[self->size] = 0x0;
	}
}

int Datum_equals_(Datum *self, Datum *other)
{
	return (self->size == other->size) && 
		(memcmp(self->data, other->data, self->size) == 0);
}

int Datum_equalsCString_(Datum *self, const char *s)
{
	return (self->size == strlen(s)) && 
		(memcmp(self->data, s, self->size) == 0);
}

unsigned int Datum_hash1(Datum *self)
{
	return MurmurHash2((const void *)self->data, (int)self->size, 0);
}

unsigned int Datum_hash2(Datum *self)
{
	return SuperFastHash((const char *)self->data, (int)self->size);
}

int Datum_beginsWith_(Datum *self, Datum *other)
{
	return (self->size >= other->size) && 
		(memcmp(self->data, other->data, other->size) == 0);
}

int Datum_beginsWithCString_(Datum *self, const char *s)
{
	return (self->size >= strlen(s)) && 
		(memcmp(self->data, s, strlen(s)) == 0);
}

int Datum_isBeginningOfCString_(Datum *self, const char *s)
{
	return (self->size <= strlen(s)) && 
		(memcmp(self->data, s, self->size) == 0);
}

void Datum_clear(Datum *self)
{
	self->data[0] = 0x0;
	self->size = 0;
}

int Datum_isEmpty(Datum *self)
{
	return self->size == 0;
}

void Datum_makeIdOfLength_(Datum *self, int length)
{
	char *cset = "0123456789abcdefghijklmnopqrstuvwxysABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int max = strlen(cset);
	int i;
	
	Datum_setSize_(self, length);
	
	for (i = 0; i < length; i ++)
	{
		self->data[i] = cset[rand() % max];
	}
}

void Datum_makePidOfLength_(Datum *self, int length)
{
	char *cset = "0123456789";
	int max = strlen(cset);
	int i;
	int prefixLength = 0;
	
	Datum_setSize_(self, length + prefixLength);
	
	for (i = prefixLength; i < length + prefixLength; i ++)
	{
		self->data[i] = cset[rand() % max];
	}
}

void Datum_makePid32(Datum *self)
{
	char s[64];
	unsigned int r = rand();

	snprintf(s, 64, "%u", r);
	Datum_setCString_(self, s);
}

int Datum_asInt(Datum *self)
{
	return atoi(self->data);
}

long Datum_asLong(Datum *self)
{
	return atol(self->data);
}

void Datum_fromLong_(Datum *self, long n)
{
	char ns[128];
	snprintf(ns, 127, "%u", (unsigned int)n);
	Datum_setCString_(self, ns);
}

int Datum_sepOnChars_with_(Datum *self, const char *s, Datum *other)
{
	char *p = strpbrk(self->data, s);
	char c;
	int len;
	
	if (!p) return -1;
	c = *p;
	
	len = p - self->data;
	Datum_setCString_(other, p + 1);
	Datum_setSize_(self, len);
	
	return c;
}

int Datum_containsChar_(Datum *self, char c)
{
     return strchr(self->data, c) != 0x0;
}

int Datum_charAt_(Datum *self, unsigned int index)
{
	if(index < self->size)
	{
		return self->data[index];
	}
	
	return -1;
}

int Datum_at_put_(Datum *self, unsigned int index, char c)
{
	if(index < self->size)
	{
		self->data[index] = c;
		return 0;
	}
	
	return -1;
}

void Datum_clipLastPathComponent(Datum *self)
{	
	char *p = strpbrk(self->data, "/");
	if (p) Datum_setSize_(self, p - self->data);
}

int Datum_from_beforeChar_into_(Datum *self, unsigned int start, char c, Datum *other)
{
	if (start < self->size)
	{
		char *s = self->data + start;
		char *m = strchr(s, c);
		int len;
		
		if (m != NULL)
		{
			len = m - (self->data + start);
		}
		else
		{
			len = self->size - start;
		}
		
		Datum_setData_size_(other, s, len);
		Datum_nullTerminate(other);
		return start + len;

	}
	
	return -1;
}

/*
int Datum_indexOfChar_(Datum *self, char c)
{
	
}
*/
