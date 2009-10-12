#include "PQuery.h"
#include "PDB.h"

#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PQUERY_ADDOP(name) CHash_at_put_(self->ops, Datum_newWithCString_(#name ""), (void *)PQuery_op_##name);

PQuery *PQuery_new(void)
{
	PQuery *self = calloc(1, sizeof(PQuery));

	return self;
}

void PQuery_free(PQuery *self)
{
	free(self);
}

void PQuery_setNode_(PQuery *self, void *node)
{
	self->node = node;
}

PNode *PQuery_node(PQuery *self)
{
	return self->node;
}

void PQuery_setTmpNode_(PQuery *self, void *node)
{
	self->tmpNode = node;
}

void PQuery_setId_(PQuery *self, Datum *d)
{
	self->id = d;
}

void PQuery_setAfter_(PQuery *self, Datum *d)
{
	self->after = d;
}

void PQuery_setBefore_(PQuery *self, Datum *d)
{
	self->before = d;
}

void PQuery_setWhereKey_(PQuery *self, Datum *d)
{
	self->whereKey = d;
}

void PQuery_setWhereValue_(PQuery *self, Datum *d)
{
	self->whereValue = d;
}

void PQuery_setSelectCountMax_(PQuery *self, unsigned int n)
{
	self->selectCountMax = n;
}

int PQuery_setup(PQuery *self)
{
	PDB *pdb = PNode_pdb(self->node);
	self->tmpNode = PDB_allocNode(pdb);
	
	self->hasFilter = self->whereKey && self->whereValue && 
		(!Datum_isEmpty(self->whereKey)) && (!Datum_isEmpty(self->whereValue));
	
	if (self->selectCountMax == 0) self->selectCountMax = 100000;
		
	PNode_first(self->node);
	
	if(self->id && Datum_size(self->id))
	{
		PNode_jump_(self->node, self->id);
	}
	else if(self->after && Datum_size(self->after))
	{
		PNode_jump_(self->node, self->after);
		if (!Datum_isEmpty(self->after)) PNode_next(self->node);
	} 
	else if(self->before && Datum_size(self->before))
	{
		PNode_jump_(self->node, self->before);
		if (!Datum_isEmpty(self->before)) PNode_previous(self->node);
	}
	
	self->selectCount = 0;
	
	return 0;
}

/*
int PQuery_run(PQuery *self)
{
	PQuery_setup(self);

	if(!Datum_isEmpty(self->op))
	{ 
		PQueryMethod *opMethod = (PQueryMethod *)CHash_at_(self->ops, self->op);
		
		if (opMethod)
		{
			return opMethod(self);
		}
	}
	
	return -1;
}
*/

void PQuery_step(PQuery *self)
{
	if(self->before && !Datum_isEmpty(self->before))
	{
		PNode_previous(self->node);
	}
	else
	{
		PNode_next(self->node);
	}
}

void PQuery_enumerate(PQuery *self)
{
	if (self->selectCount < self->selectCountMax)
	{		
		for(;;)
		{			
			PQuery_step(self);
			if(!PNode_key(self->node)) break;

			if (!(self->hasFilter) ||
				PNode_withId_hasKey_andValue_(self->tmpNode, 
					PNode_value(self->node), self->whereKey, self->whereValue))
			{		
				self->selectCount ++;
				return;
			}
		}
	}
	
	return;
}

unsigned int PQuery_selectCount(PQuery *self)
{
	return self->selectCount;
}
