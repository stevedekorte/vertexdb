#include "StoreCursor.h"

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
	free(self);
}

void StoreCursor_setDb_(StoreCursor *self, void *db)
{
	self->db = db;
}


void StoreCursor_open(StoreCursor *self)
{
	self->cursor = tcbdbcurnew(self->db);
}

void StoreCursor_close(StoreCursor *self)
{
	tcbdbcurdel(self->cursor);
}

void StoreCursor_next(StoreCursor *self)
{
	tcbdbcurnext(self->cursor);
}

void StoreCursor_previous(StoreCursor *self)
{
	tcbdbcurprev(self->cursor);
}

void StoreCursor_jump(StoreCursor *self, char *k, size_t ksize)
{
	tcbdbcurjump(self->cursor, k, ksize);
}

void StoreCursor_remove(StoreCursor *self)
{
	tcbdbcurout(self->cursor);
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