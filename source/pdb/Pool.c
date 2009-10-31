#define Pool_C
#include "Pool.h"
#undef Pool_C

#include <stdlib.h>
#include <time.h>

static Pool *globalPool = 0x0;

Pool *Pool_globalPool(void)
{
	if(!globalPool) 
	{
		globalPool = Pool_new();
	}
	
	return globalPool;
}

void Pool_globalPoolFreeRefs(void)
{
	Pool_freeRefs(Pool_globalPool());
}

void Pool_freeGlobalPool(void)
{
	if(globalPool)
	{
		Pool_free(globalPool);
		globalPool = 0x0;
	}
}

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

void Pool_freeRefsThatHaveFreeFunc_(Pool *self, PoolFreeFunc *func)
{
	List *freeRefsList = List_new();
	List *freeFuncsList = List_new();
	
	List *keepRefsList = List_new();
	List *keepFuncsList = List_new();
	
	LIST_FOREACH(self->refs, i, ref,
		PoolFreeFunc *freeFunc = List_at_(self->freeFunctions, i);
		if(freeFunc == func)
		{
			List_append_(freeRefsList, ref);
			List_append_(freeFuncsList, func);
		}
		else
		{
			List_append_(keepRefsList, ref);
			List_append_(keepFuncsList, func);
		}
	);
	
	List_copy_(self->refs, freeRefsList);
	List_copy_(self->freeFunctions, freeFuncsList);
	
	Pool_freeRefs(self);
	
	List_copy_(self->refs, keepRefsList);
	List_copy_(self->freeFunctions, keepFuncsList);	

	List_free(freeRefsList);
	List_free(freeFuncsList);
	
	List_free(keepRefsList);
	List_free(keepFuncsList);
}

void *Pool_alllocWithNewAndFree(Pool *self, PoolNewFunc *newFunc, PoolFreeFunc *freeFunc)
{
	void *ref = newFunc();
	List_append_(self->refs, ref);
	List_append_(self->freeFunctions, freeFunc);
	return ref;
}
