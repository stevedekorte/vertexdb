#ifndef Pool_DEFINED
#define Pool_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "List.h"

typedef void *(PoolNewFunc)(void);
typedef void (PoolFreeFunc)(void *);
typedef void (PoolClearFunc)(void *);

typedef struct
{
	List *refs;
	PoolNewFunc *newFunc;
	PoolFreeFunc *freeFunc;
	PoolClearFunc *clearFunc;
	List *recycled;
	int recycleSize;
} Pool;

Pool *Pool_new(void);
void Pool_free(Pool *self);
void Pool_freeRefs(Pool *self);
void Pool_setNewFunc_(Pool *self, PoolNewFunc *func);
void Pool_setFreeFunc_(Pool *self, PoolFreeFunc *func);
void Pool_setClearFunc_(Pool *self, PoolClearFunc *func);
void Pool_setRecycleSize_(Pool *self, int size);
void *Pool_newItem(Pool *self);

#ifdef __cplusplus
}
#endif
#endif
