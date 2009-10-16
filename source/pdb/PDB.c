/*
Notes:
	tcbdboptimize() only works on hash dbs, so we can't use it - use collectgarbage instead
*/

#include "PDB.h"
#include "PQuery.h"
#include "Log.h"
#include "Datum.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PDB_USE_TX 1
//#define PDB_USE_SYNC 1

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
	self->dbFile = File_new();
	self->isOpenFile = File_new();
	self->lastBackupFile = File_new();
	self->newBackupFile = File_new();
	self->corruptFile = File_new();
	self->unusedPid = Datum_new();
	self->useBackups = 1;
	self->pool = Pool_new();
	return self;
}

void PDB_setYajl_(PDB *self, yajl_gen y)
{
	self->yajl = y;
}

void PDB_free(PDB *self)
{
	PDB_freeNodes(self);
	
	PDB_close(self);
	File_free(self->dbFile);
	File_free(self->isOpenFile);
	File_free(self->lastBackupFile);
	File_free(self->newBackupFile);
	File_free(self->corruptFile);
	Datum_free(self->unusedPid);
	Pool_free(self->pool);
	self->yajl = 0x0;
	free(self);
}

// node caching

PNode *PDB_allocNode(PDB *self)
{
	PNode *node = POOL_ALLOC(self->pool, PNode);
	PNode_setPdb_(node, self);
	PNode_setToRoot(node);
	assert(self->yajl);
	PNode_setYajl_(node, self->yajl);
	return node;
}

Datum *PDB_allocDatum(PDB *self)
{
	return POOL_ALLOC(self->pool, Datum);
}

void PDB_freeNodes(PDB *self)
{
	Pool_freeRefs(self->pool);
}

// open/close ------------

void PDB_setPathCString_(PDB *self, const char *path)
{
	File_setPathCString_(self->dbFile, path);
	
	Datum_setCString_(File_path(self->isOpenFile), path);
	Datum_appendCString_(File_path(self->isOpenFile), ".open");
	
	Datum_setCString_(File_path(self->lastBackupFile), path);
	Datum_appendCString_(File_path(self->lastBackupFile), ".lastBackup");

	Datum_setCString_(File_path(self->corruptFile), path);
	Datum_appendCString_(File_path(self->corruptFile), ".corrupt");
}

void PDB_createRootIfNeeded(PDB *self)
{
	int size;
	const char *p = "1/m/size"; // 1 == root node
	
	if(!tcbdbget(self->db, p, strlen(p), &size))
	{
		PDB_begin(self);
		tcbdbput(self->db, p, strlen(p), "0", 1);
		PDB_commit(self);
	}
}
	
int PDB_open(PDB *self)
{
	self->db = tcbdbnew();

	if (!tcbdbsetcmpfunc(self->db, pathCompare, NULL))
	{
		Log_Printf("tcbdbsetcmpfunc failed\n");
		return -1;
	}
	
	tcbdbsetxmsiz(self->db, 1024*1024*100);
	
	//commented out until our server has a reasonable amount of ram
	if (!tcbdbsetcache(self->db, 1024*10, 512*10))
	{
		Log_Printf("tcbdbsetcache failed\n");
		return -1;
	}
	
	if (Datum_isEmpty(File_path(self->dbFile)))
	{
		PDB_setPathCString_(self, "db.tc");
	}
	
	if (self->useBackups)
	{
		if (File_exists(self->isOpenFile))
		{
			Log_Printf("PDB: Found isOpen file - database was not closed properly, assuming corrupt and replacing with last backup\n");
			PDB_replaceDbWithLastBackUp(self);
		}
	}
	
	{
		int flags = BDBOWRITER | BDBOCREAT | BDBONOLCK;
		
		#ifdef PDB_USE_SYNC
		flags |= BDBOTSYNC;
		#endif
		
		if (!tcbdbopen(self->db, File_pathCString(self->dbFile), flags ))
		{
			Log_Printf("tcbdbopen failed\n");
			return -1;
		}
	}
	
	//PDB_createRootIfNeeded(self);
	if (self->useBackups)
	{
		File_create(self->isOpenFile);
		
		if (!PDB_hasLastBackup(self)) 
		{
			Log_Printf("PDB: no backup found - creating one to be safe\n");
			PDB_backup(self);
		}
	}
	
	return 0;
}

void PDB_close(PDB *self)
{
	if (self->db)
	{
		PDB_abort(self);
		tcbdbclose(self->db);
		self->db = 0x0;		
		if (self->useBackups) File_remove(self->isOpenFile);
	}
}

// backups ------------

void PDB_setUseBackups_(PDB *self, int aBool)
{
	self->useBackups = aBool;
}

void PDB_setNewBackupPath(PDB *self) // caller free's it
{
	time_t rawtime;
	struct tm *timeinfo;
	char dateString[100];
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(dateString, 100, "%Y_%m_%d_%H_%M_%S", timeinfo);
	
	File_setPath_(self->newBackupFile, File_path(self->dbFile));
	File_appendToPathCString_(self->newBackupFile, "_backup_");
	File_appendToPathCString_(self->newBackupFile, dateString);
}

int PDB_backup(PDB *self) // returns "true" if successful
{
	int result;
	const char *path;
	
	if (!self->useBackups) return 1;
	
	PDB_commit(self);
	PDB_setNewBackupPath(self);
	path = Datum_data(File_path(self->newBackupFile));
	Log_Printf_("PDB creating backup: %s\n", path);
	result = tcbdbcopy(self->db, path); //tc will create a .wal file
	File_symbolicallyLinkTo_(self->newBackupFile, self->lastBackupFile);
	return !result;
}

// clean shutdown ------------

int PDB_hasLastBackup(PDB *self)
{
	return File_exists(self->lastBackupFile);
}

int PDB_replaceDbWithLastBackUp(PDB *self)
{
	if (PDB_hasLastBackup(self))
	{
		File_moveTo_(self->dbFile, self->corruptFile);
		File_copyTo_(self->lastBackupFile, self->dbFile);
	}
	else
	{
		Log_Printf("PDB: no last backup file - unable to replace corrupt db\n");
	}
	
	return 0;
}

void PDB_fatalError_(PDB *self, char *s)
{
	Log_Printf__("%s failed: %s\n", s, tcbdberrmsg(tcbdbecode(self->db)));
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
	if (!tcbdbtranbegin(self->db))
	{
		PDB_fatalError_(self, "tcbdbtranbegin");
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
	if (!tcbdbtranabort(self->db))
	{
		PDB_fatalError_(self, "tcbdbtranabort");
	}
	#endif

	self->inTransaction = 0;
}

void PDB_commit(PDB *self)
{
	time_t now;
	
	if (!self->inTransaction) return;

	now = time(NULL);
	
	Log_Printf("committing...\n");

	#ifdef PDB_USE_TX
	if (!tcbdbtrancommit(self->db))
	{
		PDB_fatalError_(self, "tcbdbtrancommit");
	}
	#else
	if(!tcbdbsync(self->db))
	{
		PDB_fatalError_(self, "tcbdbsync");
	}
	#endif
	
	Log_Printf___("commit took %i seconds, %i records, %iMB\n", 
		(int)difftime(time(NULL), now),
		(int)tcbdbrnum(self->db), 
		(int)(tcbdbfsiz(self->db)/(1024*1024)));
	
	self->writeByteCount = 0;
	self->inTransaction = 0;
}

// read/write ------------

void *PDB_at_(PDB *self, const char *k, int ksize, int *vsize)
{
	void *v = tcbdbget(self->db, k, ksize, vsize);
	//PDB_fatalError_(self, "tcbdbput");
	return v;
}

int PDB_at_put_(PDB *self, const char *k, int ksize, const char *v, int vsize)
{
	PDB_willWrite(self);
	
	if(!tcbdbput(self->db, k, ksize, v, vsize))
	{
		PDB_fatalError_(self, "tcbdbput");
		return 0;
	}

	self->writeByteCount += ksize;
	self->writeByteCount += vsize;

	return 1;
}

int PDB_at_cat_(PDB *self, const char *k, int ksize, const char *v, int vsize)
{
	PDB_willWrite(self);
	
	if(!tcbdbputcat(self->db, k, ksize, v, vsize))
	{
		PDB_fatalError_(self, "tcbdbput");
		return 0;
	}

	self->writeByteCount += ksize;
	self->writeByteCount += vsize;

	return 1;
}


int PDB_removeAt_(PDB *self, const char *k, int ksize)
{
	PDB_willWrite(self);

	if(!tcbdbout(self->db, k, ksize))
	{
		PDB_fatalError_(self, "tcbdbout");
		return 0;
	}

	self->writeByteCount += ksize;

	return 1;
}

size_t PDB_bytesWaitingToCommit(PDB *self)
{
	return self->writeByteCount;
}

// nodes ------------

PNode *PDB_newNode(PDB *self)
{
	PNode *p = PNode_new();
	PNode_setPdb_(p, self);
	return p;
}

int PDB_syncSizes(PDB *self)
{
	int max = 100000;
	int sizesChanged = 0;
	int nodeCount = 0;
	PNode *p = PDB_newNode(self);
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
	
	PNode_free(p);
	PDB_commit(self);
	
	printf("nodeCount:    %i\n", nodeCount);
	printf("sizesChanged: %i\n", sizesChanged);
	return sizesChanged;
}

// garbage collection -----------------------------------------------------

long PDB_saveMarkedNodes(PDB *self)
{
	size_t savedCount = 0;
	PNode *inNode = PDB_newNode(self);
	PNode *outNode;
	PDB *out = PDB_new();
	
	PDB_setUseBackups_(out, 0);
	PDB_setPathCString_(out, "/tmp/db.tc");
	PDB_open(out);
	
	outNode = PDB_newNode(out);
	
	Log_Printf_("  PDB copying %i marked nodes to new db...\n", 
		(int)CHash_count(self->markedPids));

	
	CHASH_FOREACH(self->markedPids, k, v, 
		PNode_setPidLong_(inNode, (long)k);
		PNode_setPidLong_(outNode, (long)k);
		PNode_first(inNode);
		
		while(PNode_key(inNode))
		{
			PNode_atPut_(outNode, PNode_key(inNode), PNode_value(inNode));
			PNode_next(inNode);
		}
		
		savedCount ++;
		
		if(savedCount % 10000 == 0) 
		{
			Log_Printf_("    %i\n", (int)savedCount);
		}
	);
	
	PNode_free(inNode);
	PNode_free(outNode);
	
	PDB_commit(out);
	PDB_close(out);
	PDB_close(self);
	
	File_remove(self->dbFile);
	File_moveTo_(out->dbFile, self->dbFile);
	
	PDB_open(self);

	return savedCount;
}

void PDB_markReachableNodes(PDB *self)
{
	size_t i = 0;
	PNode *aNode = PDB_newNode(self);
	
	PDB_addToMarkQueue_(self, 1); // root node
	
	Log_Printf("  PDB marking nodes...\n");
	
	while(List_size(self->markQueue) != 0)
	{
		long pid = (long)List_pop(self->markQueue);
		PNode_setPidLong_(aNode, pid);
		PNode_mark(aNode);
		i++;
		if (i % 10000 == 0) { Log_Printf_("    %i\n", (int)i); }
	}
	
	PNode_free(aNode);
}

long PDB_sizeInMB(PDB *self)
{
	return (long)(tcbdbfsiz(self->db)/(1024*1024));
}

#include "Hash_murmur.h"
#include "Hash_superfast.h"

int Pointer_equals_(void *p1, void *p2)
{
	uintptr_t i1 = (uintptr_t)p1;
	uintptr_t i2 = (uintptr_t)p2;
	if(i1 == i2) return 0;
	if(i1 < i2) return 1;
	return -1;
}

unsigned int Pointer_hash1(void *p)
{
	return MurmurHash2((const void *)&p, (int)sizeof(void *), 0);
}

unsigned int Pointer_hash2(void *p)
{
	return SuperFastHash((const char *)&p, (int)sizeof(void *));

}

long PDB_collectGarbage(PDB *self)
{
	long collectedCount = 0;

	Log_Printf_("PDB collectGarbage, %iMB before collect:\n", (int)PDB_sizeInMB(self));

	self->markedPids = CHash_new();
	CHash_setEqualFunc_(self->markedPids, (CHashEqualFunc *)Pointer_equals_);
	CHash_setHash1Func_(self->markedPids, (CHashHashFunc *)Pointer_hash1);
	CHash_setHash2Func_(self->markedPids, (CHashHashFunc *)Pointer_hash2);
	
	self->markQueue  = List_new();
	
	PDB_markReachableNodes(self);
	//collectedCount = PDB_removeUnmarkedNodes(self);
	collectedCount = PDB_saveMarkedNodes(self);
	
	List_free(self->markQueue); self->markQueue = 0x0;
	CHash_free(self->markedPids); self->markedPids = 0x0;
	
	PDB_commit(self);
	Log_Printf_("  %iMB after collect:\n", (int)PDB_sizeInMB(self));
	return collectedCount;
}

int PDB_hasMarked_(PDB *self, long pid)
{
	return CHash_at_(self->markedPids, (void *)pid) != 0x0;
}

void PDB_addToMarkQueue_(PDB *self, long pid)
{
	if (CHash_at_(self->markedPids, (void *)pid) == 0x0)
	{
		List_append_(self->markQueue, (void *)pid);
		CHash_at_put_(self->markedPids, (void *)pid, (void *)0x1);
	}
}

/*
void PDB_removeBackups(PDB *self)
{

}
*/

void PDB_remove(PDB *self)
{
	PDB_close(self);
	if(File_exists(self->dbFile)) File_remove(self->dbFile);
}


