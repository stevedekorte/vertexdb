
#ifndef Pnode_DEFINED
#define Pnode_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>  
#include <sys/time.h>  
//#include <sys/queue.h>
#include <stdlib.h>  

#include <err.h>  
#include <event.h>  
#include <evhttp.h>  

#include <tcutil.h>
#include <tcbdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "Datum.h"
#include "PQuery.h"

typedef struct
{
	void *pdb;
	BDBCUR *cursor;
	
	Datum *pid;
	Datum *pidPath;
	Datum *key;
	Datum *keyPath;
	Datum *value;
	
	Datum *sizePath;
	Datum *parentPid;
	PQuery *query;
} PNode;

typedef int (PNodeOp)(PNode *, Datum *);

PNode *PNode_new(void);
void PNode_free(PNode *self);

void PNode_setPdb_(PNode *self, void *pdb);
void PNode_open(PNode *self);
void PNode_close(PNode *self);
void *PNode_pdb(PNode *self);
PQuery *PNode_query(PNode *self);

void PNode_setPid_(PNode *self, Datum *pid);
void PNode_setPidLong_(PNode *self, long pid);
void PNode_setPidCString_(PNode *self, const char *pid);
Datum *PNode_pid(PNode *self);

Datum *PNode_at_(PNode *self, Datum *k);
Datum *PNode_atCString_(PNode *self, const char *k);
int PNode_atPut_(PNode *self, Datum *k, Datum *v);
int PNode_atCat_(PNode *self, Datum *k, Datum *v);
int PNode_removeAt_(PNode *self, Datum *k);
void PNode_setToRoot(PNode *self);

int PNode_setSize_(PNode *self, long s); // PRIVATE !!!
void PNode_setPathsFromPid(PNode *self);
long PNode_size(PNode *self);
int PNode_incrementSize(PNode *self);
int PNode_decrementSize(PNode *self);
long PNode_nodeSizeAtCursor(PNode *self);

int PNode_doesExist(PNode *self);
void PNode_first(PNode *self);
void PNode_jump_(PNode *self, Datum *k);
int PNode_next(PNode *self);
int PNode_previous(PNode *self);
void PNode_removeAtCursor(PNode *self);

void PNode_setKey_(PNode *self, Datum *v);
Datum *PNode_key(PNode *self);

void PNode_setValue_(PNode *self, Datum *v);
Datum *PNode_value(PNode *self);

Datum *PNode_unusedKey(PNode *self);

int PNode_deref_(PNode *self, Datum *key);
int PNode_moveToKey_(PNode *self, Datum *key);
int PNode_createMoveToKeyString_(PNode *self, const char *k);
int PNode_createMoveToKey_(PNode *self, Datum *key);
int PNode_mergeTo_(PNode *self, PNode *destNode, int withKeys);
int PNode_removeTo_(PNode *self, Datum *k);
int PNode_remove(PNode *self);
Datum *PNode_valueFromDerefKeyToPath_(PNode *self, Datum *derefPath);

int PNode_moveToPathIfExists_(PNode *self, Datum *p);
int PNode_moveToPath_(PNode *self, Datum *p);
int PNode_moveToPathCString_(PNode *self, const char *p);
//int PNode_moveToSubpathCString_(PNode *self, const char *p);

void PNode_create(PNode *self);

// garbage collection helpers

long PNode_pidAsLong(PNode *self);
int PNode_isMarked(PNode *self);
void PNode_mark(PNode *self);

int PNode_takePidFromCursor(PNode *self); // returns 0 on success
int PNode_moveToNextNode(PNode *self); // returns slot count or a negative number on end or error
int PNode_findSize(PNode *self); // returns slot count or a negative number on end or error

int PNode_withId_hasKey_andValue_(PNode *self, Datum *pid, Datum *wk, Datum *wv);

// ------------------

int PNode_op_json(PNode *self, Datum *d);
int PNode_op_counts(PNode *self, Datum *d);
int PNode_op_keys(PNode *self, Datum *d);
int PNode_op_pairs(PNode *self, Datum *d);
int PNode_op_values(PNode *self, Datum *d);
int PNode_op_rm(PNode *self, Datum *d);

Datum *PNode_op_owner(PNode *self);
Datum *PNode_op_public(PNode *self);

#ifdef __cplusplus
}
#endif
#endif
