
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
#include "Yajl_extras.h"
#include "Store.h"
//#include "PCollector.h"
//#include "PBackup.h"

typedef void  (PDBPutCallback)(void *, const char *k, int ksize, const char *v, int vsize);
typedef void  (PDBCatCallback)(void *, const char *k, int ksize, const char *v, int vsize);
typedef void  (PDBRemoveCallback)(void *, const char *k, int ksize);

typedef struct
{
	Store *store;
	File *dbFile;
	int inTransaction;
	//Datum *currentUser;		
	yajl_gen yajl;
	int isClosing;
	int hardSync;
	void *collector;
	
	// callbacks
	void *delegate;
	PDBPutCallback *putCallback;
	PDBCatCallback *catCallback;
	PDBRemoveCallback *removeCallback;
} PDB;

PDB *PDB_new(void);
void PDB_setYajl_(PDB *self, yajl_gen yajl);
void PDB_free(PDB *self);

// callbacks
void PDB_setDelegate_(PDB *self, void *d);
void PDB_setPutCallback_(PDB *self, PDBPutCallback *func);
void PDB_setCatCallback_(PDB *self, PDBCatCallback *func);
void PDB_setRemoveCallback_(PDB *self, PDBRemoveCallback *func);

// nodes -------------
PNode *PDB_allocNode(PDB *self);
PNode *PDB_newNode(PDB *self);

// open/close ------------
void PDB_setPathCString_(PDB *self, const char *path);
void PDB_setHardSync_(PDB *self, int aBool);
int PDB_open(PDB *self);
void PDB_close(PDB *self);

// backups ------------
void PDB_setUseBackups_(PDB *self, int aBool);
int PDB_backup(PDB *self);

// transactions ------------
void PDB_willWrite(PDB *self); // private to PDB and PNode
void PDB_begin(PDB *self);
void PDB_abort(PDB *self);
void PDB_rawCommit(PDB *self); // private
void PDB_commit(PDB *self);

// read/write ------------
void *PDB_at_(PDB *self, const char *k, int ksize, int *vsize);
int PDB_at_put_(PDB *self, const char *k, int ksize, const char *v, int vsize);
int PDB_at_cat_(PDB *self, const char *k, int ksize, const char *v, int vsize);
int PDB_removeAt_(PDB *self, const char *k, int ksize);

// helpers ------------
long PDB_sizeInMB(PDB *self);
uint64_t PDB_numberOfKeys(PDB *self);
int PDB_syncSizes(PDB *self);

void PDB_beginCollectGarbage(PDB *self);
int PDB_isCollecting(PDB *self);
void PDB_collectStep(PDB *self);

void PDB_moveTo_(PDB *self, PDB *other);

void PDB_remove(PDB *self);

#ifdef __cplusplus
}
#endif
#endif
