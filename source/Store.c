/*
Notes:
	tcbdboptimize() only works on hash dbs, so we can't use it - use collectgarbage instead
*/

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

void Store_setPath_(Store *self, const char *p)
{
	self->path = strcpy(realloc(self->path, strlen(p)+1), p);
}

const char *Store_path(Store *self)
{
	return self->path;
}

void Store_setCompareFunction_(Store *self, void *func)
{
	self->compareFunc = func;
}

void Store_setHardSync_(Store *self, int aBool)
{
	self->hardSync = aBool;
}


int Store_open(Store *self)
{
	self->db = tcbdbnew();

	/*
	//Tinkering with these seems to result in worse performance so far...
	
	tcbdbsetxmsiz(self->db, 1024*1024*64); 

	if(!tcbdbtune(self->db, 0, 0, 0, -1, -1, BDBTDEFLATE)) // HDBTLARGE
	{
		Log_Printf("tcbdbtune failed\n");
		return -1;
	}

	if (!tcbdbsetcache(self->db, 1024*100, 512*100))
	{
		Log_Printf("tcbdbsetcache failed\n");
		return -1;
	}
	*/
	
	if (!tcbdbsetcmpfunc(self->db, self->compareFunc, NULL))
	{
		return 0;
	}

	{
		int flags = BDBOWRITER | BDBOCREAT | BDBONOLCK;
		
		if (self->hardSync) 
		{
			printf("Store: hard disk syncing enabled\n");
			flags |= BDBOTSYNC;
		}
		
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

int Store_backup(Store *self, const char *backupPath)
{
	return tcbdbcopy(self->db, backupPath); //tc will create a .wal file
}

const char *Store_error(Store *self)
{
	return tcbdberrmsg(tcbdbecode(self->db));
}

char *Store_read(Store *self, const char *k, size_t ksize, int *vsize)
{

	void *v = tcbdbget(self->db, k, ksize, vsize);
	return v;
}

int Store_write(Store *self, const char *k, size_t ksize, const char *v, size_t vsize)
{
	if(!tcbdbput(self->db, k, ksize, v, vsize))
	{
		return 0;
	}
	
	return 1;
}

int Store_append(Store *self, const char *k, size_t ksize, const char *v, size_t vsize)
{
	if(!tcbdbputcat(self->db, k, ksize, v, vsize))
	{
		return 0;
	}
	
	return 1;
}

int Store_remove(Store *self, const char *k, size_t ksize)
{
	if(!tcbdbout(self->db, k, ksize))
	{
		return 0;
	}
	
	return 1;
}

int Store_sync(Store *self)
{
	if(!tcbdbsync(self->db))
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

int Store_size(Store *self)
{
	return (long)(tcbdbfsiz(self->db));
}

/*
StoreCursor *Store_newCursor(Store *self)
{

}
*/
	/*
	if(!tcbdbsync(self->db))
	{
		PDB_fatalError_(self, "tcbdbsync");
	}
	*/
	
	/*
	Log_Printf___("commit took %i seconds, %i records, %iMB\n", 
		(int)difftime(time(NULL), now),
		(int)tcbdbrnum(self->db), 
		(int)(tcbdbfsiz(self->db)/(1024*1024)));
	*/
	
/*
void PDB_warmup(PDB *self)
{
	// touch all the indexes to pull them into memory
	BDBCUR *c = tcbdbcurnew(self->db);
	tcbdbcurfirst(c);
	while(tcbdbcurnext(c)) {}
	tcbdbcurdel(c);
}
*/

