
#include "PDB.h"
#include "PQuery.h"
#include "Log.h"
#include "Datum.h"
#include "Date.h"
#include "Pointer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "PCollector.h"

#define PDB_USE_TX 1

static int pathCompareBase(const char *p1, int len1, const char *p2, int len2, void *optionalOpaqueValue)
{
	int i;
	int len = len1 < len2 ? len1 : len2;
	char sepChar = '/';
	
	for (i = 0; i < len; i ++)
	{
		char c1 = p1[i];
		char c2 = p2[i];
		
		if (c1 == sepChar && c2 != sepChar)
		{
			return -1;
		}
  		else if (c2 == sepChar && c1 != sepChar)
		{
			return 1;
		}
		
		if (c1 > c2) return 1;
		if (c1 < c2) return -1;
	}
	
	if (len1 > len2) return 1;
	if (len1 < len2) return -1;
	
	return 0;
}

static int pathCompare(const char *p1, int len1, const char *p2, int len2, void *optionalOpaqueValue)
{
	int r1 = pathCompareBase(p1, len1, p2, len2, optionalOpaqueValue);
	int r2 = pathCompareBase(p2, len2, p1, len1, optionalOpaqueValue);
	
	if ((r1 || r2) && (r1 == r2))
	{
		printf("PDB: error in pathCompare!\n");
		exit(-1);
	}
	
	return r1;
}

PDB *PDB_new(void)
{
	PDB *self = calloc(1, sizeof(PDB));
	self->store = Store_new();
	self->dbFile = File_new();
	self->hardSync = 1;
	srand(time(NULL)); // need to do because Datum_makePid64 uses rand 
	
	return self;
}

void PDB_setYajl_(PDB *self, yajl_gen y)
{
	self->yajl = y;
}

void PDB_free(PDB *self)
{	
	PDB_close(self);
	Store_free(self->store);
	File_free(self->dbFile);
	if(self->collector) PCollector_free(self->collector);
	self->yajl = 0x0;
	free(self);
}

// callbacks

void PDB_setDelegate_(PDB *self, void *d)
{
	self->delegate = d;
}

void PDB_setPutCallback_(PDB *self, PDBPutCallback *func)
{
	self->putCallback = func;
}

void PDB_setCatCallback_(PDB *self, PDBCatCallback *func)
{
	self->catCallback = func;
}

void PDB_setRemoveCallback_(PDB *self, PDBRemoveCallback *func)
{
	self->removeCallback = func;
}

// node caching

PNode *PDB_allocNode(PDB *self)
{
	PNode *node = PNode_poolNew();
	PNode_setPdb_(node, self);
	PNode_setToRoot(node);
	PNode_setYajl_(node, self->yajl);
	return node;
}

PNode *PDB_newNode(PDB *self)
{
	PNode *node = PNode_new();
	PNode_setPdb_(node, self);
	PNode_setToRoot(node);
	PNode_setYajl_(node, self->yajl);
	return node;
}

// open/close ------------

void PDB_setPathCString_(PDB *self, const char *path)
{
	File_setPathCString_(self->dbFile, path);
	
	/*
	Datum_setCString_(File_path(self->isOpenFile), path);
	Datum_appendCString_(File_path(self->isOpenFile), ".open");
	
	Datum_setCString_(File_path(self->lastBackupFile), path);
	Datum_appendCString_(File_path(self->lastBackupFile), ".lastBackup");

	Datum_setCString_(File_path(self->corruptFile), path);
	Datum_appendCString_(File_path(self->corruptFile), ".corrupt");
	*/
}

void PDB_setHardSync_(PDB *self, int aBool)
{
	self->hardSync = aBool;
}

void PDB_createRootIfNeeded(PDB *self)
{
	int size;
	const char *p = "1/m/size"; // 1 == root node
	char *v = Store_read(self->store, (char *)p, strlen(p), &size);
	
	if(!v)
	{
		PDB_begin(self);
		Store_write(self->store, (char *)p, strlen(p), "0", 1);
		PDB_rawCommit(self);
	}
	else 
	{
		free(v);
	}

}
	
int PDB_open(PDB *self)
{
	Store_setCompareFunction_(self->store, pathCompare);
	Store_setHardSync_(self->store, self->hardSync);
	
	if (Datum_isEmpty(File_path(self->dbFile)))
	{
		PDB_setPathCString_(self, "db.tc");
	}
	
	/*
	if (self->useBackups)
	{
		if (File_exists(self->isOpenFile))
		{
			Log_Printf("PDB: Found isOpen file - database was not closed properly, assuming corrupt and replacing with last backup...\n");
			PDB_replaceDbWithLastBackUp(self);
			Log_Printf("PDB: finished backup replace\n");
		}
	}
	*/
	
	{
		Store_setPath_(self->store, File_pathCString(self->dbFile));
		if (!Store_open(self->store))
		{
			Log_Printf("open failed\n");
			return -1;
		}
	}
	
	//PDB_createRootIfNeeded(self);
	/*
	if (self->useBackups)
	{
		File_create(self->isOpenFile);
		Log_Printf("PDB: making startup backup\n");
		PDB_backup(self);
	}
	*/
	
	return 0;
}

int PDB_backup(PDB *self)
{
	return 0;
}


void PDB_close(PDB *self)
{
	if(!self->isClosing)
	{
		self->isClosing = 1;
		PDB_rawCommit(self); // right thing to do?
		//PDB_abort(self);
		Log_Printf("PDB: closing...\n");
		Store_close(self->store);
		Log_Printf("PDB: closed\n");
		//if (self->useBackups) File_remove(self->isOpenFile);
		self->isClosing = 0;
	}
}

// backups ------------

void PDB_setUseBackups_(PDB *self, int aBool)
{
	//self->useBackups = aBool;
}

void PDB_fatalError_(PDB *self, char *s)
{
	Log_Printf__("%s failed: %s\n", s, Store_error(self->store));
	PDB_close(self);
	exit(-1);
}

// transactions ------------

void PDB_willWrite(PDB *self)
{
	if (!self->inTransaction) PDB_begin(self);
}

void PDB_begin(PDB *self)
{
	if(self->inTransaction) return;
	
	#ifdef PDB_USE_TX
	if (!Store_begin(self->store))
	{
		PDB_fatalError_(self, "transaction begin");
	}
	else
	#endif
	{
		self->inTransaction = 1;
	}
}

void PDB_abort(PDB *self)
{
	if (!self->inTransaction) return;

	#ifdef PDB_USE_TX
	if (!Store_abort(self->store))
	{
		PDB_fatalError_(self, "transaction abort");
	}
	#endif

	self->inTransaction = 0;
}


void PDB_rawCommit(PDB *self)
{
	time_t now;
	
	if (!self->inTransaction) return;

	now = time(NULL);
	
	//Log_Printf("committing...\n");

	#ifdef PDB_USE_TX
	if (!Store_commit(self->store))
	{
		PDB_fatalError_(self, "transaction commit");
	}
	#endif

	self->inTransaction = 0;
}

void PDB_commit(PDB *self)
{
	PDB_rawCommit(self);
}

// read/write ------------

void *PDB_at_(PDB *self, const char *k, int ksize, int *vsize)
{
	void *v = Store_read(self->store, k, ksize, vsize);
	return v;
}

int PDB_at_put_(PDB *self, const char *k, int ksize, const char *v, int vsize)
{
	PDB_willWrite(self);
	
	if(!Store_write(self->store, k, ksize, v, vsize))
	{
		PDB_fatalError_(self, "write");
		return 0;
	}
	
	if(self->putCallback)
	{
		self->putCallback(self->delegate, k, ksize, v, vsize);
	}

	return 1;
}

int PDB_at_cat_(PDB *self, const char *k, int ksize, const char *v, int vsize)
{
	PDB_willWrite(self);
	
	if(!Store_append(self->store, k, ksize, v, vsize))
	{
		PDB_fatalError_(self, "put");
		return 0;
	}
	
	if(self->catCallback)
	{
		self->catCallback(self->delegate, k, ksize, v, vsize);
	}

	return 1;
}


int PDB_removeAt_(PDB *self, const char *k, int ksize)
{
	PDB_willWrite(self);

	if(!Store_remove(self->store, k, ksize))
	{
		PDB_fatalError_(self, "remove");
		return 0;
	}

	if(self->removeCallback)
	{
		self->removeCallback(self->delegate, k, ksize);
	}
	
	return 1;
}

// sync ------------

int PDB_sync(PDB *self)
{
	PDB_commit(self);
	
	if(!Store_sync(self->store))
	{
		PDB_fatalError_(self, "sync");
	}
	
	return 0;
}

int PDB_syncSizes(PDB *self)
{
	int max = 100000;
	int sizesChanged = 0;
	int nodeCount = 0;
	PNode *p = PDB_allocNode(self);
	PNode_open(p);
	
	PNode_setPidCString_(p, "");
	printf("pid: '%s'\n", Datum_data(PNode_pid(p)));
	PNode_moveToNextNode(p);
	printf("pid: '%s'\n", Datum_data(PNode_pid(p)));

	while (PNode_moveToNextNode(p) == 0)
	{
		int size = PNode_size(p);
		int findSize = PNode_findSize(p);
	
		//printf("pid: '%s'\n", Datum_data(PNode_pid(p)));

		if (size != findSize)
		{
		    /*
			printf("pid: '%s'\n", Datum_data(PNode_pid(p)));
			printf("	size:  %i\n", size);
			printf("	fsize: %i\n", findSize);
			*/
			PNode_setSize_(p, findSize);
			sizesChanged ++;
		}
		
		nodeCount ++;
		
		if (nodeCount > max)
		{
			printf("exceeded max node count - some kind of loop?\n");
			break;
		}
	}
	
	PDB_commit(self);
	
	printf("nodeCount:    %i\n", nodeCount);
	printf("sizesChanged: %i\n", sizesChanged);
	return sizesChanged;
}

void PDB_beginCollectGarbage(PDB *self)
{	
	if(!self->collector)
	{
		self->collector = PCollector_new();
		PCollector_setIn_(self->collector, self);
		PCollector_begin(self->collector);
	}
}

int PDB_isCollecting(PDB *self)
{
	return self->collector && PCollector_isCollecting(self->collector);
}

void PDB_collectStep(PDB *self)
{
	if(self->collector) PCollector_step(self->collector);
}

// ---------------------------------------------------------------------------

long PDB_sizeInMB(PDB *self)
{
	return (long)(Store_size(self->store)/(1024*1024));
}

void PDB_remove(PDB *self)
{
	PDB_close(self);
	if(File_exists(self->dbFile)) File_remove(self->dbFile);
}

void PDB_moveTo_(PDB *self, PDB *other)
{
	File_remove(other->dbFile);
	File_moveTo_(self->dbFile, other->dbFile);
}

