#define Pool_C
#include "Pool.h"
#undef Pool_C

#include <stdlib.h>
#include <time.h>

Pool *Pool_new(void)
{
	Pool *self = calloc(1, sizeof(Pool));
	self->refs = List_new();
	self->recycled = List_new();
	return self;
}

void Pool_free(Pool *self)
{
	LIST_FOREACH(self->refs, i, ref, self->freeFunc(ref););
	List_free(self->refs);
	
	LIST_FOREACH(self->recycled, i, ref, self->freeFunc(ref););
	List_free(self->recycled);
	
	free(self);
}


void Pool_freeRecycled(Pool *self)
{
	LIST_FOREACH(self->recycled, i, ref, self->freeFunc(ref););
	List_removeAll(self->recycled);
}

void Pool_freeRefs(Pool *self)
{
	PoolFreeFunc *freeFunc = self->freeFunc;
	
	LIST_FOREACH(self->refs, i, ref, 
		if(List_size(self->recycled) < self->recycleSize)
		{
			self->clearFunc(ref);
			List_append_(self->recycled, ref);
		}
		else
		{
			freeFunc(ref);
		}
	);
	
	List_removeAll(self->refs);
}

void Pool_setNewFunc_(Pool *self, PoolNewFunc *f)
{
	self->newFunc = f;
}

void Pool_setFreeFunc_(Pool *self, PoolFreeFunc *f)
{
	self->freeFunc = f;
}

void Pool_setClearFunc_(Pool *self, PoolClearFunc *f)
{
	self->clearFunc = f;
}

void Pool_setRecycleSize_(Pool *self, int size)
{
	self->recycleSize = size;
}

void *Pool_newItem(Pool *self)
{
	void *ref;
	
	if(List_size(self->recycled))
	{
		ref = List_pop(self->recycled);
	}
	else 
	{
		ref = self->newFunc();
	}

	List_append_(self->refs, ref);
	return ref;
}

void Pool_showStats(Pool *self)
{
	printf("pool total: %i refs: %i recycled: %i recycleSize: %i\n", 
		(int)List_size(self->refs) + (int)List_size(self->recycled),
		(int)List_size(self->refs), 
		(int)List_size(self->recycled), 
		(int)self->recycleSize);
}

