#ifndef Pool_DEFINED
#define Pool_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "List.h"

typedef struct
{
	List *refs;
	List *freeFunctions;
} Pool;

typedef void *(PoolNewFunc)(void);
typedef void (PoolFreeFunc)(void *);

Pool *Pool_globalPool(void);
void Pool_globalPoolFreeRefs(void);
void Pool_freeGlobalPool(void);

Pool *Pool_new(void);
void Pool_free(Pool *self);
void Pool_freeRefs(Pool *self);
void Pool_freeRefsThatHaveFreeFunc_(Pool *self, PoolFreeFunc *func);
void *Pool_alllocWithNewAndFree(Pool *self, PoolNewFunc *newFunc, PoolFreeFunc *freeFunc);

#define POOL_ALLOC(self, name) Pool_alllocWithNewAndFree(self, (PoolNewFunc *)name##_new, (PoolFreeFunc *)name##_free)
#define GLOBAL_POOL_ALLOC(name) POOL_ALLOC(Pool_globalPool(), name);

#ifdef __cplusplus
}
#endif
#endif
