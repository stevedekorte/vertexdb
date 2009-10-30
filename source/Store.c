#include "Store.h"

#include <tcutil.h>
#include <tcbdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

Store *Store_new(void)
{
	Store *self = calloc(1, sizeof(Store));
	Store_setPath_(self, "default.store");
	return self;
}

void Store_free(Store *self)
{
	free(self->path);
	free(self);
}

void Store_setPath_(Store *self, char *p)
{
	self->path = strcpy(realloc(self->path, strlen(p)+1), p);
}

char *Store_path(Store *self)
{
	return self->path;
}

void Store_setCompareFunction_(Store *self, void *func)
{
	self->compareFunc = func;
}

int Store_open(Store *self)
{
	self->db = tcbdbnew();

	if (!tcbdbsetcmpfunc(self->db, self->compareFunc, NULL))
	{
		return 0;
	}

	{
		int flags = BDBOWRITER | BDBOCREAT | BDBONOLCK; //BDBOTSYNC
		
		if (!tcbdbopen(self->db, self->path, flags))
		{
			return 0;
		}
	}
	
	return 1;
}

int Store_close(Store *self)
{
	return tcbdbclose(self->db);
}

const char *Store_error(Store *self)
{
	return tcbdberrmsg(tcbdbecode(self->db));
}

char *Store_read(Store *self, char *k, size_t ksize, int *vsize)
{

	void *v = tcbdbget(self->db, k, ksize, vsize);
	return v;
}

int Store_write(Store *self, char *k, size_t ksize, char *v, size_t vsize)
{
	if(!tcbdbput(self->db, k, ksize, v, vsize))
	{
		return 0;
	}
	
	return 1;
}

int Store_append(Store *self, char *k, size_t ksize, char *v, size_t vsize)
{
	if(!tcbdbputcat(self->db, k, ksize, v, vsize))
	{
		return 0;
	}
	
	return 1;
}

int Store_begin(Store *self)
{
	if (!tcbdbtranbegin(self->db))
	{
		return 0;
	}
	
	return 1;
}

int Store_abort(Store *self)
{
	if (!tcbdbtranabort(self->db))
	{
		return 0;
	}

	return 1;
}

int Store_commit(Store *self)
{
	if (!tcbdbtrancommit(self->db))
	{
		return 0;
	}
	
	return 1;
}

/*
StoreCursor *Store_newCursor(Store *self)
{

}
*/
