
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
	self->dbFile = File_new();
	self->isOpenFile = File_new();
	self->lastBackupFile = File_new();
	self->newBackupFile = File_new();
	self->corruptFile = File_new();
	//self->unusedPid = Datum_new();
	self->useBackups = 0;
	self->store = Store_new();
	self->marksPerStep = 1000;
	self->maxStepTime = .1;
	
	srand(time(NULL)); // need to do because Datum_makePid64 uses rand 
	
	return self;
}

void PDB_setYajl_(PDB *self, yajl_gen y)
{
	self->yajl = y;
}

void PDB_free(PDB *self)
{	
	PDB_cancelCollectGarbage(self);
	PDB_close(self);
	Store_free(self->store);
	File_free(self->dbFile);
	File_free(self->isOpenFile);
	File_free(self->lastBackupFile);
	File_free(self->newBackupFile);
	File_free(self->corruptFile);
	//Datum_free(self->unusedPid);
	self->yajl = 0x0;
	free(self);
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
	
	if (self->useBackups)
	{
		if (File_exists(self->isOpenFile))
		{
			Log_Printf("PDB: Found isOpen file - database was not closed properly, assuming corrupt and replacing with last backup...\n");
			PDB_replaceDbWithLastBackUp(self);
			Log_Printf("PDB: finished backup replace\n");
		}
	}
	
	{
		Store_setPath_(self->store, File_pathCString(self->dbFile));
		if (!Store_open(self->store))
		{
			Log_Printf("open failed\n");
			return -1;
		}
	}
	
	//PDB_createRootIfNeeded(self);
	if (self->useBackups)
	{
		File_create(self->isOpenFile);
		
		/*
		if (!PDB_hasLastBackup(self)) 
		{
			Log_Printf("PDB: no backup found - creating one to be safe\n");
			PDB_backup(self);
		}
		*/
		
		Log_Printf("PDB: making startup backup\n");
		PDB_backup(self);

	}
	
	return 0;
}

void PDB_close(PDB *self)
{
	//if (self->store)
	if(!self->isClosing)
	{
		self->isClosing = 1;
		//PDB_cancelCollectGarbage(self);
		PDB_rawCommit(self); // right thing to do?
		//PDB_abort(self);
		Log_Printf("PDB: closing...\n");
		Store_close(self->store);
		Log_Printf("PDB: closed\n");
		if (self->useBackups) File_remove(self->isOpenFile);
		self->isClosing = 0;
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
	
	PDB_rawCommit(self);
	PDB_setNewBackupPath(self);
	path = Datum_data(File_path(self->newBackupFile));
	Log_Printf_("PDB creating backup: %s\n", path);
	result = Store_backup(self->store, path);
	File_symbolicallyLinkTo_(self->newBackupFile, self->lastBackupFile);
	Log_Printf("PDB done creating backup\n");
	self->lastBackupTime = time(NULL);
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

	
	self->writeByteCount = 0;
	self->inTransaction = 0;
}

void PDB_commit(PDB *self)
{
	PDB_rawCommit(self);
	
	PDB_markReachableNodesStepIfNeeded(self);
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
	
	// if the collector is active, treat all new nodes as referenced
	if (k[0] != '_') // then v is a pointer
	{
		if (PDB_isCollecting(self))
		{
			long pid = atol(v);
			PDB_addToMarkQueue_(self, pid);
		}
	}

	self->writeByteCount += ksize;
	self->writeByteCount += vsize;

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

	self->writeByteCount += ksize;
	self->writeByteCount += vsize;

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
		
		savedCount ++;
		//printf("copying key %i saved count %i\n", (int)k, (int)savedCount);
		 
		while(PNode_key(inNode))
		{
			PNode_atPut_(outNode, PNode_key(inNode), PNode_value(inNode));
			PNode_next(inNode);
		}
		
		if(savedCount % 1000 == 0) 
		{
			// Free Datum pools periodically to avoid eating too much RAM
			Log_Printf_("    %i\n", (int)savedCount);
			Datum_poolFreeRefs();
			PDB_rawCommit(out);
		}
	);
	
	PNode_free(inNode);
	PNode_free(outNode);
	
	PDB_rawCommit(out);
	PDB_close(out);
	PDB_close(self);
	
	File_remove(self->dbFile);
	File_moveTo_(out->dbFile, self->dbFile);
	PDB_free(out);
	
	PDB_open(self);

	if(savedCount != (long)CHash_count(self->markedPids))
	{
		Log_Printf__("  PDB saved count of %i does not match markedPids count of %i\n", 
			(int)savedCount, (int)CHash_count(self->markedPids));
	}
		
	return savedCount;
}

void PDB_showMarkStatus(PDB *self)
{
	Log_Printf__(" collector queued: %i marked: %i\n", 
		(int)List_size(self->markQueue),	
		(int)self->markCount
	); 
	
	//printf("Datum ");
	//Pool_showStats(Datum_pool());
	/*
	Pool_freeRefs(Datum_pool());
	Pool_freeRecycled(Datum_pool());
	printf("PNode ");
	Pool_showStats(PNode_pool());
	Pool_freeRefs(PNode_pool());
	Pool_freeRecycled(PNode_pool());
	*/
	
	printf("CHash markedPids size: %0.2fM\n", 
		((float)self->markedPids->size)/1000000.0);
	//printf("datumCount: %i\n", Datum_datumCount());
	printf("\n");
}

void PDB_incrementMarkCount(PDB *self)
{
	self->markCount ++;
	
	if (self->markCount % 10000 == 0) 
	{ 
		PDB_showMarkStatus(self);
	}

	/*
	if (self->markCount % 10000 == 0)
	{ 
		PDB_reopenDuringCollectGarbage(self);
	}
	*/
}

void PDB_markPid_(PDB *self, long pid)
{
	PNode_setPidLong_(self->tmpMarkNode, pid);
	PNode_mark(self->tmpMarkNode);
	PDB_incrementMarkCount(self);
}

void PDB_markReachableNodesStepIfNeeded(PDB *self)
{
	if(PDB_isCollecting(self))
	{
		PDB_markReachableNodesStep(self);
	}
}

void PDB_markReachableNodesStep(PDB *self)
{
	double t1 = Date_SecondsFrom1970ToNow();
	int count = 0;
	double dt = 0;
	
	while ((dt < self->maxStepTime) && (count < self->marksPerStep))
	{
		long pid = (long)List_pop(self->markQueue);
		PDB_markPid_(self, pid);
		count ++;
		
		if (List_size(self->markQueue) == 0)
		{
			PDB_completeCollectGarbage(self);
			break;
		}
		
		dt = Date_SecondsFrom1970ToNow() - t1;
	}
}

int PDB_isCollecting(PDB *self)
{
	return (self->markedPids != 0x0);
}

void PDB_beginCollectGarbage(PDB *self)
{
	if(PDB_isCollecting(self)) 
	{
		PDB_markReachableNodesStep(self);
		return;
	}
	
	Log_Printf_("PDB: beginCollectGarbage %iMB before collect\n", (int)PDB_sizeInMB(self));

	self->markedPids = CHash_new();
	CHash_setEqualFunc_(self->markedPids, (CHashEqualFunc *)Pointer_equals_);
	CHash_setHash1Func_(self->markedPids, (CHashHashFunc *)Pointer_hash1);
	CHash_setHash2Func_(self->markedPids, (CHashHashFunc *)Pointer_hash2);
	
	self->markQueue  = List_new();
	self->markCount = 0;
	self->tmpMarkNode = PDB_newNode(self);
	
	PDB_addToMarkQueue_(self, 1); // root node
}

long PDB_completeCollectGarbage(PDB *self)
{
	long savedCount = PDB_saveMarkedNodes(self);
	Log_Printf_("PDB: completeCollectGarbage %iMB after collect\n", (int)PDB_sizeInMB(self));
	PDB_cleanUpCollectGarbage(self);
	return savedCount;
}

void PDB_cancelCollectGarbage(PDB *self)
{	
	PDB_cleanUpCollectGarbage(self);
}

void PDB_cleanUpCollectGarbage(PDB *self)
{	
	if(self->markQueue)
	{
		List_free(self->markQueue); 
		self->markQueue = 0x0;
	}
	
	if(self->markedPids)
	{
		CHash_free(self->markedPids); 
		self->markedPids = 0x0;
	}
	
	if(self->tmpMarkNode)
	{
		PNode_free(self->tmpMarkNode);
		self->tmpMarkNode = 0x0;
	}
}

void PDB_reopenDuringCollectGarbage(PDB *self)
{
	printf("PDB_reopenDuringCollectGarbage\n");
	
	if(self->tmpMarkNode)
	{
		PNode_free(self->tmpMarkNode);
		self->tmpMarkNode = 0x0;
	}
	
	PDB_close(self);
	PDB_open(self);
	
	self->tmpMarkNode = PDB_newNode(self);
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

// ---------------------------------------------------------------------------

long PDB_sizeInMB(PDB *self)
{
	return (long)(Store_size(self->store)/(1024*1024));
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

