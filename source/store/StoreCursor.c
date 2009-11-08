#include "StoreCursor.h"
#include "Store.h"

#include <tcutil.h>
#include <tcbdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

StoreCursor *StoreCursor_new(void)
{
	StoreCursor *self = calloc(1, sizeof(StoreCursor));

	return self;
}

void StoreCursor_free(StoreCursor *self)
{
	StoreCursor_close(self);
	free(self);
}

void StoreCursor_setStore_(StoreCursor *self, void *store)
{
	self->store = store;
}


void StoreCursor_open(StoreCursor *self)
{
	self->cursor = tcbdbcurnew(((Store *)self->store)->db);
}

void StoreCursor_close(StoreCursor *self)
{
	if(self->cursor)
	{
		tcbdbcurdel(self->cursor);
		self->cursor = 0x0;
	}
}

int StoreCursor_next(StoreCursor *self)
{
	return tcbdbcurnext(self->cursor);
}

int StoreCursor_previous(StoreCursor *self)
{
	return tcbdbcurprev(self->cursor);
}

void StoreCursor_jump(StoreCursor *self, const char *k, size_t ksize)
{
	tcbdbcurjump(self->cursor, k, ksize);
}

int StoreCursor_remove(StoreCursor *self)
{
	return tcbdbcurout(self->cursor);
}

char *StoreCursor_key(StoreCursor *self, int *size)
{
	return tcbdbcurkey(self->cursor, size);
}

char *StoreCursor_value(StoreCursor *self, int *size)
{
	return tcbdbcurval(self->cursor, size);
}


//void StoreCursor_first(StoreCursor *self);
//void StoreCursor_last(StoreCursor *self)