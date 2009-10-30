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

void *PQuery_tmpNode(PQuery *self)
{
	return (void *)self->tmpNode;
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

void PQuery_setAttribute_(PQuery *self, Datum *d)
{
	self->attribute = d;
}

Datum *PQuery_attribute(PQuery *self)
{
	return self->attribute;
}

Datum *PQuery_attributeValue(PQuery *self)
{
	return PNode_at_(self->tmpNode, self->attribute);
}

int PQuery_setup(PQuery *self)
{
	PDB *pdb = PNode_pdb(self->node);
	self->tmpNode = PDB_allocNode(pdb);
	self->isDone = 0;
	
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
		Datum *k;
		PNode_jump_(self->node, self->after);
		k = PNode_key(self->node);
		if (k && Datum_equals_(self->after, k)) PNode_next(self->node);
	} 
	else if(self->before && Datum_size(self->before))
	{
		Datum *k;
		PNode_jump_(self->node, self->before);
		k = PNode_key(self->node);
		if (k && Datum_equals_(self->before, k)) PNode_previous(self->node);
	}
	
	if(!PQuery_cursorMatches(self))
	{
		PQuery_moveToNextMatch(self);
	}
	
	self->selectCount = 1;
	
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

int PQuery_stepDirection(PQuery *self)
{
	if(self->before && !Datum_isEmpty(self->before))
	{
		return -1;
	}

	return 1;
}

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

Datum *PQuery_key(PQuery *self)
{
	if (self->isDone) return 0x0;
	return PNode_key(self->node);
}

int PQuery_cursorMatches(PQuery *self)
{
	return !(self->hasFilter) || PNode_withId_hasKey_andValue_(self->tmpNode, 
				PNode_value(self->node), self->whereKey, self->whereValue);
}

int PQuery_moveToNextMatch(PQuery *self) // return 1 if found, 0 if reached end
{
	for(;;)
	{			
		PQuery_step(self);
		
		if(!PNode_key(self->node)) break;

		if (PQuery_cursorMatches(self))
		{
			return 1;
		}
	}
	
	return 0;
}

void PQuery_enumerate(PQuery *self)
{
	if (self->selectCount < self->selectCountMax)
	{
		
		int found = PQuery_moveToNextMatch(self);
		
		if (found)
		{
				self->selectCount ++;	
		}
		else
		{
			self->isDone = 1;
		}
	}
	else 
	{
		self->isDone = 1;
	}

	
	return;
}

unsigned int PQuery_selectCount(PQuery *self)
{
	return self->selectCount;
}
