
#ifndef Pnode_DEFINED
#define Pnode_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>  
#include <sys/time.h>  
#include <stdlib.h>  

#include "Store.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "Datum.h"
#include "PQuery.h"
#include "Yajl_extras.h"

typedef struct
{
	void *pdb;
	StoreCursor *storeCursor;
	
	Datum *pid;
	Datum *pidPath;
	Datum *keyPath;
	
	Datum *sizePath;
	Datum *parentPid;
	PQuery *query;
	
	yajl_gen yajl;
} PNode;

typedef int (PNodeOp)(PNode *, Datum *);


// creation and setup
PNode *PNode_poolNew(void);
PNode *PNode_new(void);
void PNode_poolFreeRefs(void);
void PNode_freePool(void);

void PNode_setYajl_(PNode *self, yajl_gen y);
void PNode_setPdb_(PNode *self, void *pdb);
void PNode_free(PNode *self);
void PNode_clear(PNode *self);

// open/close
void PNode_open(PNode *self);
void PNode_close(PNode *self);
void *PNode_pdb(PNode *self);
PQuery *PNode_query(PNode *self);

// pids
void PNode_setPid_(PNode *self, Datum *pid);
void PNode_setPidLong_(PNode *self, long pid);
void PNode_setPidCString_(PNode *self, const char *pid);
Datum *PNode_pid(PNode *self);

// at, put, remove
Datum *PNode_at_(PNode *self, Datum *k);
Datum *PNode_atCString_(PNode *self, const char *k);
int PNode_atPut_(PNode *self, Datum *k, Datum *v);
int PNode_atCat_(PNode *self, Datum *k, Datum *v);
int PNode_removeAt_(PNode *self, Datum *k);
void PNode_setToRoot(PNode *self);

// meta
int PNode_metaAt_put_(PNode *self, Datum *key, Datum *value);
Datum *PNode_metaAt_(PNode *self, Datum *d);

// size ops
int PNode_setSize_(PNode *self, long s); // PRIVATE !!!
void PNode_setPathsFromPid(PNode *self);
long PNode_size(PNode *self);
int PNode_incrementSize(PNode *self);
int PNode_decrementSize(PNode *self);
long PNode_nodeSizeAtCursor(PNode *self);

// query enumeration
PQuery *PNode_startQuery(PNode *self);
int PNode_doesExist(PNode *self);
void PNode_first(PNode *self);
void PNode_jump_(PNode *self, Datum *k);
void PNode_jumpToCurrentKey(PNode *self);
int PNode_next(PNode *self);
int PNode_previous(PNode *self);
void PNode_removeAtCursor(PNode *self);

// set/get key and value
void PNode_setKey_(PNode *self, Datum *v);
Datum *PNode_key(PNode *self);
void PNode_setValue_(PNode *self, Datum *v);
Datum *PNode_value(PNode *self);

// paths
int PNode_moveToKey_(PNode *self, Datum *key);
int PNode_createMoveToKeyString_(PNode *self, const char *k);
int PNode_createMoveToKey_(PNode *self, Datum *key);
//int PNode_mergeTo_(PNode *self, PNode *destNode, int withKeys);
int PNode_remove(PNode *self);
//Datum *PNode_valueFromDerefKeyToPath_(PNode *self, Datum *derefPath);
int PNode_moveToPathIfExists_(PNode *self, Datum *p);
int PNode_moveToSubpathIfExists_(PNode *self, Datum *p);
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

// query ops
int PNode_op_object(PNode *self, Datum *d);
int PNode_op_sizes(PNode *self, Datum *d);
int PNode_op_count(PNode *self, Datum *d);
int PNode_op_keys(PNode *self, Datum *d);
int PNode_op_pairs(PNode *self, Datum *d);
int PNode_op_values(PNode *self, Datum *d);
int PNode_op_rm(PNode *self, Datum *d);
int PNode_op_html(PNode *self, Datum *d);
int PNode_asHtmlRow(PNode *self, Datum *d);
int PNode_amSeries(PNode *self, Datum *d);
int PNode_amGraphKey_(PNode *self, Datum *title, Datum *graphKey, Datum *d);

// permissions
Datum *PNode_op_owner(PNode *self);
Datum *PNode_op_public(PNode *self);

// debugging
void PNode_show(PNode *self);

#ifdef __cplusplus
}
#endif
#endif
