#define Pool_C
#include "Pool.h"
#undef Pool_C

#include <stdlib.h>
#include <time.h>

Pool *Pool_new(void)
{
	Pool *self = calloc(1, sizeof(Pool));
	self->refs = List_new();
	self->freeFunctions = List_new();
	return self;
}

void Pool_free(Pool *self)
{
	Pool_freeRefs(self);
	List_free(self->refs);
	List_free(self->freeFunctions);
	free(self);
}

void Pool_freeRefs(Pool *self)
{
	LIST_FOREACH(self->refs, i, ref,
		PoolFreeFunc *freeFunc = List_at_(self->freeFunctions, i);
		freeFunc(ref);
	);
	List_removeAll(self->refs);
	List_removeAll(self->freeFunctions);
}

void *Pool_alllocWithNewAndFree(Pool *self, PoolNewFunc *newFunc, PoolFreeFunc *freeFunc)
{
	void *ref = newFunc();
	List_append_(self->refs, ref);
	List_append_(self->freeFunctions, freeFunc);
	return ref;
}
