
#ifndef PDB_DEFINED
#define PDB_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "PNode.h"
#include "List.h"
#include "File.h"
#include "Common.h"
#include "CHash.h"
#include "Pool.h"
#include <yajl/yajl_gen.h>

typedef struct
{
	File *dbFile;
	File *isOpenFile;
	File *lastBackupFile;
	File *newBackupFile;
	File *corruptFile;
	
	TCBDB *db;
	Datum *unusedPid;
	int inTransaction;
	size_t writeByteCount;
	
	// garbage collection
	CHash *markedPids;
	List *markQueue;
	int useBackups;
	
	Datum *currentUser;
		
	Pool *pool;
	yajl_gen jsonGenerator;
} PDB;

PDB *PDB_new(void);
void PDB_setYajl_(PDB *self, yajl_gen jsonGenerator);
void PDB_free(PDB *self);

// node caching -------------
PNode *PDB_allocNode(PDB *self);
Datum *PDB_allocDatum(PDB *self);
void PDB_freeNodes(PDB *self);

// open/close ------------
void PDB_setPathCString_(PDB *self, const char *path);
int PDB_open(PDB *self);
void PDB_close(PDB *self);

// clean shutdown ------------
void PDB_setUseBackups_(PDB *self, int aBool);
int PDB_replaceDbWithLastBackUp(PDB *self);
int PDB_backup(PDB *self);
int PDB_hasLastBackup(PDB *self);

// transactions ------------
void PDB_willWrite(PDB *self); // private to PDB and PNode
void PDB_begin(PDB *self);
void PDB_abort(PDB *self);
void PDB_commit(PDB *self);

// read/write ------------
void *PDB_at_(PDB *self, const char *k, int ksize, int *vsize);
int PDB_at_put_(PDB *self, const char *k, int ksize, const char *v, int vsize);
int PDB_at_cat_(PDB *self, const char *k, int ksize, const char *v, int vsize);
int PDB_removeAt_(PDB *self, const char *k, int ksize);
size_t PDB_bytesWaitingToCommit(PDB *self);

// nodes ------------
PNode *PDB_newNode(PDB *self); // caller responsible for freeing returned PNode
int PDB_syncSizes(PDB *self);

// garbage collection ------------
long PDB_collectGarbage(PDB *self); // returns # of nodes collected
int PDB_hasMarked_(PDB *self, long pid);
void PDB_addToMarkQueue_(PDB *self, long pid);


#ifdef __cplusplus
}
#endif
#endif
