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

Pool *Pool_new(void);
void Pool_free(Pool *self);
void Pool_freeRefs(Pool *self);
void *Pool_alllocWithNewAndFree(Pool *self, PoolNewFunc *newFunc, PoolFreeFunc *freeFunc);

#define POOL_ALLOC(self, name) Pool_alllocWithNewAndFree(self, (PoolNewFunc *)name##_new, (PoolFreeFunc *)name##_free)

#ifdef __cplusplus
}
#endif
#endif
