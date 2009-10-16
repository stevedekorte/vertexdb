#include "PNode.h"
#include "PDB.h"
#include "PQuery.h"

#include <sys/types.h>  
#include <sys/time.h>  
#include <sys/queue.h>
#include <stdlib.h>  

#include <tcutil.h>
#include <tcbdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define PNODE_ID_LENGTH 9

PNode *PNode_new(void)
{
	PNode *self = calloc(1, sizeof(PNode));
	self->pid           = Datum_new(); // pid
	self->pidPath       = Datum_new(); // pid/m/
	self->key           = Datum_new(); // key
	self->keyPath       = Datum_new(); // pid/m/key
	self->value         = Datum_new();
	self->sizePath      = Datum_new();
	self->parentPid     = Datum_new();
	
	return self;
}

void PNode_setYajl_(PNode *self, yajl_gen y)
{
	self->yajl = y;
}

void PNode_free(PNode *self)
{
	PNode_close(self);
	Datum_free(self->pid);
	Datum_free(self->pidPath);
	Datum_free(self->key);
	Datum_free(self->keyPath);
	Datum_free(self->value);
	Datum_free(self->sizePath);
	Datum_free(self->parentPid);
	
	if (self->query) 
	{
		PQuery_free(self->query);
		self->query = 0x0;
	}
	
	self->yajl = 0x0;
	
	free(self);
}

PQuery *PNode_query(PNode *self)
{
	if(!self->query)
	{
		self->query = PQuery_new();
		PQuery_setNode_(self->query, self);
	}
	
	return self->query;
}

void *PNode_pdb(PNode *self)
{
	return self->pdb;
}

PQuery *PNode_startQuery(PNode *self)
{
	PQuery *q = PNode_query(self);
	PQuery_setup(q);
	return q;
}

void PNode_setPdb_(PNode *self, void *pdb)
{
	self->pdb = pdb;
	PNode_open(self);
}

void PNode_open(PNode *self)
{
	self->cursor = tcbdbcurnew(((PDB *)self->pdb)->db);
}

void PNode_close(PNode *self)
{
	if (self->cursor)
	{
		tcbdbcurdel(self->cursor);
		self->cursor = 0x0;
	}
}

void PNode_setPid_(PNode *self, Datum *pid)
{
	PNode_setPidCString_(self, Datum_data(pid));
}

void PNode_setPidLong_(PNode *self, long pid)
{
	char k[64];
	sprintf(k, "%u", (unsigned int)pid);
	PNode_setPidCString_(self, k);
}

void PNode_setPidCString_(PNode *self, const char *pid)
{
	Datum_setCString_(self->pid, pid);
	PNode_setPathsFromPid(self);
}

Datum *PNode_pid(PNode *self)
{
	return self->pid;
}

Datum *PNode_parentPid(PNode *self)
{
	return self->parentPid;
}

Datum *PNode_at_(PNode *self, Datum *k)
{
	return PNode_atCString_(self, Datum_data(k));
}

Datum *PNode_atCString_(PNode *self, const char *k)
{
	int size;
	void *value;
	
	Datum_copy_(self->keyPath, self->pidPath);
	Datum_appendCString_(self->keyPath, k);
	 
	value = PDB_at_(self->pdb, Datum_data(self->keyPath), (int)Datum_size(self->keyPath), &size);
	
	
	if (value != 0x0)
	{
		Datum_setData_size_(self->value, value, size);
		free(value);
		//printf("key = '%s' value = '%s'\n", Datum_data(self->keyPath), Datum_data(self->value));
		return self->value;
	}
	
	//printf("key = '%s' value = %p\n", Datum_data(self->keyPath), (void *)value);
	
	return 0x0;
}

// --------- mutation ---------

int PNode_atPut_(PNode *self, Datum *k, Datum *v)
{
	int hasKey = PNode_at_(self, k) != NULL;

	if (!PDB_at_put_(self->pdb, 
		Datum_data(self->keyPath), (int)Datum_size(self->keyPath), 
		Datum_data(v), (int)Datum_size(v)))
	{
		printf("PNode_atPut_ error\n");
		return -1;
	}

	if (!hasKey) return PNode_incrementSize(self);
	
	return 0;
}

int PNode_atCat_(PNode *self, Datum *k, Datum *v)
{
	int hasKey = PNode_at_(self, k) != NULL;

	if (!PDB_at_cat_(self->pdb, 
		Datum_data(self->keyPath), (int)Datum_size(self->keyPath), 
		Datum_data(v), (int)Datum_size(v)))
	{
		printf("PNode_atCat_ error\n");
		return -1;
	}

	if (!hasKey) return PNode_incrementSize(self);
	
	return 0;
}

int PNode_removeAt_(PNode *self, Datum *k)
{
	if ((!Datum_isEmpty(k)) && (PNode_at_(self, k) != 0x0))
	{
		if(!PDB_removeAt_(self->pdb, Datum_data(self->keyPath), (int)Datum_size(self->keyPath)))
		{
			printf("PNode_removeAt_ error\n");
			return -1;
		}

		return PNode_decrementSize(self);
	}
	
	return 0;
}

// ------------------------------

void PNode_setPathsFromPid(PNode *self)
{
	Datum_copy_(self->sizePath, self->pid);
	Datum_appendCString_(self->sizePath, "/m/size");
	
	Datum_copy_(self->pidPath, self->pid);
	Datum_appendCString_(self->pidPath, "/s/");
}

long PNode_nodeSizeAtCursor(PNode *self)
{
	// carefull - this temporarilly hacks the sizePath
	long size;
	
	Datum *otherPid = PNode_value(self);
	Datum_copy_(self->sizePath, otherPid);
	Datum_appendCString_(self->sizePath, "/m/size");
	size = PNode_size(self);
	PNode_setPathsFromPid(self);
	
	return size;
}

long PNode_size(PNode *self)
{
	int size;
	void *value;
	 
	value = PDB_at_(self->pdb, Datum_data(self->sizePath), (int)Datum_size(self->sizePath), &size);
	
	if (value)
	{
		long sizeValue = atoi(value);
		//Datum_setData_size_(self->value, value, size);
		free(value);
		//printf("%s = %i\n", Datum_data(self->sizePath), sizeValue);
		return sizeValue;
	}

	return 0;
}

int PNode_setSize_(PNode *self, long s)
{
	Datum_copy_(self->sizePath, self->pid);
	Datum_appendCString_(self->sizePath, "/m/size");
	
	{
		char ns[128];
		snprintf(ns, 127, "%u", (unsigned int)s);
		
		//printf("%s <- %s\n",  Datum_data(self->sizePath), ns);
		
		if(!PDB_at_put_(self->pdb, Datum_data(self->sizePath), (int)Datum_size(self->sizePath), (const void *)ns, (int)strlen(ns)))
		{
			return -1;
		}
	}
	
	return 0;
}

int PNode_incrementSize(PNode *self)
{
	long n = PNode_size(self) + 1;
	return PNode_setSize_(self, n);
}

int PNode_decrementSize(PNode *self)
{
	if (PNode_size(self)) 
	{
		return PNode_setSize_(self, PNode_size(self) - 1);
	}
	else
	{
		printf("PNode warning: attempt to decrement pnode size of zero\n");
	}
	
	return 0;
}

void PNode_first(PNode *self) // returns 0 when at end
{
	tcbdbcurjump(self->cursor, Datum_data(self->pidPath), (int)Datum_size(self->pidPath));
	
	if(!PNode_key(self))
	{
		tcbdbcurnext(self->cursor);
	}
}

void PNode_jump_(PNode *self, Datum *k)
{
	Datum_copy_(self->keyPath, self->pidPath);
	Datum_append_(self->keyPath, k);
	tcbdbcurjump(self->cursor, Datum_data(self->keyPath), (int)Datum_size(self->keyPath));
}

int PNode_next(PNode *self) // returns 0 when at end
{
	tcbdbcurnext(self->cursor);
	return PNode_key(self) != 0;
}

int PNode_previous(PNode *self)
{
	tcbdbcurprev(self->cursor);
	return PNode_key(self) != 0;
}

Datum *PNode_key(PNode *self)
{
	int size;
	char *ks = tcbdbcurkey(self->cursor, &size);
      
	//if (ks == 0x0 || size == 0) return 0x0;
	if (ks == 0x0) return 0x0;

	if(Datum_isBeginningOfCString_(self->pidPath, ks)) 
	{
	   const char *subpath = ks + Datum_size(self->pidPath);
	   Datum_setCString_(self->key, subpath);
	   free(ks);
	   return self->key;
	}
   
	free(ks);
	return 0x0;
}

Datum *PNode_value(PNode *self)
{
	int size;
	char *v = tcbdbcurval(self->cursor, &size);
	
	if (v) 
	{
		Datum_setData_size_(self->value, v, size);
		free(v);
		return self->value;
	}
	
	return 0x0;
}

int PNode_doesExist(PNode *self)
{
	tcbdbcurjump(self->cursor, Datum_data(self->pid), (int)Datum_size(self->pid));
	return PNode_key(self) != 0x0;
}

int PNode_createMoveToKeyString_(PNode *self, const char *k)
{
	Datum *key = Datum_newWithCString_(k);
	int r = PNode_createMoveToKey_(self, key);
	Datum_free(key);
	return r;
}

int PNode_moveToKey_(PNode *self, Datum *key)
{
	/*
	if (Datum_equalsCString_(key, "approved"))
	{
		printf("approved");
	}
	*/
	
	Datum *v = PNode_at_(self, key);
	
	if (v)
	{
		Datum_copy_(self->parentPid, self->pid);
		PNode_setPid_(self, v);
		return 0;
	}
	
	return -1;
}
	
int PNode_createMoveToKey_(PNode *self, Datum *key)
{
	Datum *v = PNode_at_(self, key);
	
	if (v)
	{
		Datum_copy_(self->parentPid, self->pid);
		PNode_setPid_(self, v);
		return 1;
	}
	else
	{
		PNode *n = PDB_newNode(self->pdb);
		PNode_create(n);
		PNode_atPut_(self, key, PNode_pid(n));
		Datum_copy_(self->parentPid, self->pid);
		PNode_setPid_(self, PNode_pid(n));
		PNode_free(n);
	}
	
	return 0;
}

/*
int PNode_mergeTo_(PNode *self, PNode *destNode, int keysOnly)
{	
	// hack: we keep self->value empty and don't fetch it if keysOnly != 0
	Datum *key;
	
	PNode_first(self);
	
	Datum_clear(self->value);
	
	while ((key = PNode_key(self)))
	{
		if (!keysOnly) PNode_value(self);
		
		if (PNode_atPut_(destNode, key, self->value))
		{
			return -1;
		}
		
		PNode_next(self);
	}
	
	return 0;
}
*/

PNode *PNode_nodeAt_(PNode *self, Datum *k)
{	
	// hack: we keep self->value empty and don't fetch it if keysOnly != 0
	Datum *v = PNode_at_(self, k);
	
	if (v)
	{
		PNode *target = PNode_new();
		PNode_setPdb_(target, self->pdb);
		PNode_setPid_(target, v);
		return target;
	}
	
	return 0x0;
}

PNode *PNode_newCopy(PNode *self, int keysOnly)
{	
	// hack: we keep self->value empty and don't fetch it if keysOnly != 0
	Datum *key;
	PNode *target = PNode_new();

	PNode_setPdb_(target, self->pdb);
	PNode_create(target);
	PNode_first(self);
	
	Datum_clear(self->value);
	
	while ((key = PNode_key(self)))
	{
		if (!keysOnly) PNode_value(self);
		PNode_atPut_(target, key, self->value);
		PNode_next(self);
	}
	
	return target;
}

int PNode_mergeTo_(PNode *self, PNode *destNode, int keysOnly)
{	
	// hack: we keep self->value empty and don't fetch it if keysOnly != 0
	Datum *key;
	
	PNode_first(self);
		
	while ((key = PNode_key(self)))
	{
		PNode *p = PNode_nodeAt_(self, key);
		
		//printf("merging %s %s\n", Datum_data(key), Datum_data(PNode_pid(p)));

		if (p)
		{
			PNode *newCopy = PNode_newCopy(p, keysOnly);
			//printf("%s atPut %s %s\n", Datum_data(PNode_pid(destNode)), Datum_data(key), Datum_data(PNode_pid(newCopy)));
			PNode_atPut_(destNode, key, PNode_pid(newCopy));
			PNode_free(newCopy);
		}
		
		PNode_free(p);
		PNode_next(self);
	}
	
	return 0;
}

void PNode_removeAtCursor(PNode *self)
{	
	if(tcbdbcurout(self->cursor))
	{
		PNode_decrementSize(self);
	}
}

int PNode_removeTo_(PNode *self, Datum *endKey)
{	
	Datum *k;
	long size = PNode_size(self);
	long removeCount = 0;
	
	while ((k = PNode_key(self)))
	{
		PDB_willWrite(self->pdb);
		if(!tcbdbcurout(self->cursor)) break;
		removeCount ++;
		if (Datum_equals_(k, endKey)) break;
	}
	
	PNode_setSize_(self, size - removeCount);
	
	return 0;
}

int PNode_remove(PNode *self)
{
	Datum *k;
	long removeCount = 0;
	
	while ((k = PNode_key(self)))
	{
		PDB_willWrite(self->pdb);
		if(!tcbdbcurout(self->cursor)) break;
		removeCount ++;
	}
	
	Datum_copy_(self->sizePath, self->pid);
	Datum_appendCString_(self->sizePath, "/m/size");

	PDB_removeAt_(self->pdb, Datum_data(self->sizePath), Datum_size(self->sizePath));
	
	return removeCount;
}

Datum *PNode_valueFromDerefKeyToPath_(PNode *self, Datum *derefPath)
{
	Datum *k = PNode_key(self);
	PNode *pn = PNode_new();

	PNode_setPdb_(pn, self->pdb);

	if (!k) goto done;
	
	PNode_moveToPath_(pn, derefPath);
	k = PNode_at_(pn, k);
	if (!k) goto done;
	
	Datum_copy_(self->value, k);
	return self->value;
	
	done:
	PNode_free(pn);
	return 0x0;
}

void PNode_setToRoot(PNode *self)
{
	PNode_setPidCString_(self, "1");
	PNode_first(self);
}

int PNode_moveToPath_createIfAbsent_(PNode *self, Datum *p, int createIfAbsent)
{
	int r;
	Datum *cp = Datum_new();
	Datum *np = Datum_new();
	
	Datum_copy_(cp, p);
	PNode_setToRoot(self);
	
	do	
	{
		r = Datum_sepOnChars_with_(cp, "/", np);
		
		if (!Datum_isEmpty(cp)) 
		{
			if (createIfAbsent)
			{
				PNode_createMoveToKey_(self, cp);
			}
			else
			{
				if (PNode_moveToKey_(self, cp)) 
				{
					Datum_free(cp);
					Datum_free(np);
					return -1;
				}
			}
		}

			
		Datum_copy_(cp, np);
	} while(r != -1);
	
	Datum_free(cp);
	Datum_free(np);
	
	return 0;
}

int PNode_moveToPathIfExists_(PNode *self, Datum *p)
{
	return PNode_moveToPath_createIfAbsent_(self, p, 0);
}

int PNode_moveToPath_(PNode *self, Datum *p)
{	
	return PNode_moveToPath_createIfAbsent_(self, p, 1);
}

int PNode_moveToPathCString_(PNode *self, const char *p)
{
	int result;
	Datum *np = Datum_newWithCString_(p);
	result = PNode_moveToPath_(self, np);
	Datum_free(np);
	return result;
}

void PNode_create(PNode *self)
{	
	int size;
	
	do
	{
		Datum_makePid32(self->pid);
		PNode_setPathsFromPid(self);
	} while (PDB_at_(self->pdb, Datum_data(self->sizePath), (int)Datum_size(self->sizePath), &size));
	
	PDB_at_put_(self->pdb, Datum_data(self->sizePath), (int)Datum_size(self->sizePath), "", 0);
}

// garbage collection helpers

long PNode_pidAsLong(PNode *self)
{
	return Datum_asLong(self->pid);
}

int PNode_isMarked(PNode *self)
{
	return PDB_hasMarked_(self->pdb, PNode_pidAsLong(self));
}

void PNode_mark(PNode *self)
{
	Datum *k;

	PNode_first(self);

	if (99900808 == Datum_asLong(self->pid))
	{
		printf("marking products node\n");
	}
	
	while ((k = PNode_key(self)))
	{
		Datum *v = PNode_value(self);
		
		if ((!Datum_beginsWithCString_(k , "_")) && strchr(Datum_data(v), '.') == 0x0) 
		// && Datum_size(v) == PNODE_ID_LENGTH)
		{
			long pid = Datum_asLong(v);

			if (pid)
			{
				if (!PDB_hasMarked_(self->pdb, pid))
				{
					//printf("mark key: %s adding to markQueue pid %i\n", Datum_data(k), pid);
					PDB_addToMarkQueue_(self->pdb, pid);
				}
			}
		}
		
		PNode_next(self);
	}
}

int PNode_takePidFromCursor(PNode *self) // returns 0 on success
{
	//long currentPid = Datum_asLong(self->pid);
	int size;
	char *ks = tcbdbcurkey(self->cursor, &size);
	
	if (ks == 0x0) return -1;

	char *slash = strchr(ks, '/');
	
	if(!slash) return -1;
	*slash = 0x0;
	
	PNode_setPidCString_(self, ks);
	free(ks);
	
	/*
	{
		long newPid = Datum_asLong(self->pid);
		int error = newPid < currentPid;
		
		if (error)
		{
			printf("error %i < %i\n", newPid, currentPid);
		}
		
		return error;
	}
	*/
	return 0;
}

int PNode_moveToNextNode(PNode *self) // returns slot count or a negative number on end or error
{
	PNode_first(self);
	
	for(;;)
	{
		if(!tcbdbcurnext(self->cursor)) return -1;
		if (PNode_key(self) == 0) break;
	}
	
	return PNode_takePidFromCursor(self);
}

int PNode_findSize(PNode *self) // returns slot count or a negative number on end or error
{
	int slotCount = 0;
	
	PNode_first(self);
	
	do
	{
		slotCount ++;
	} while (PNode_next(self));
	
	return slotCount;
}

int PNode_withId_hasKey_andValue_(PNode *self, Datum *pid, Datum *wk, Datum *wv)
{	
	Datum *value;

	PNode_setPid_(self, pid);
	value = PNode_at_(self, wk);
	return value ? Datum_equals_(value, wv) : 0; 
}

// ---------------------

int PNode_op_object(PNode *self, Datum *d)
{
	PQuery *q = PNode_startQuery(self);

	yajl_gen_map_open(self->yajl);
	
	while (PNode_key(self))
	{
		yajl_gen_datum(self->yajl, PNode_key(self));
		yajl_gen_datum(self->yajl, PNode_value(self));
		PQuery_enumerate(q);
	}
	
	yajl_gen_map_close(self->yajl);
	Datum_appendYajl_(d, self->yajl);
	return 0;
}

int PNode_op_counts(PNode *self, Datum *d)
{	
	PNode *tmpNode = PDB_allocNode(self->pdb);
	Datum *k;
	PQuery *q = PNode_startQuery(self);
	
	//Datum_appendCString_(d, "{");
	yajl_gen_map_open(self->yajl);
	
	while (k = PNode_key(self))
	{
		if(!Datum_beginsWithCString_(k, "_"))
		{			
			yajl_gen_datum(self->yajl, PNode_key(self));
		
			//Datum_appendQuoted_(d, k);
			//Datum_appendCString_(d, ":"); 
			PNode_setPid_(tmpNode, PNode_value(self));
			//Datum_appendLong_(d, PNode_size(tmpNode));
			yajl_gen_integer(self->yajl, PNode_size(tmpNode));
		}
		
		if (q) { PQuery_enumerate(q); } else { PNode_next(self); }
		//if (PNode_key(self)) Datum_appendCString_(d, ",");
	}
	
	//Datum_appendCString_(d, "}");
	yajl_gen_map_close(self->yajl);
	Datum_appendYajl_(d, self->yajl);
	return 0;  
}

int PNode_op_keys(PNode *self, Datum *d)
{	
	PQuery *q = PNode_startQuery(self);
	Datum *k;
	
	//Datum_appendCString_(d, "[");
	yajl_gen_array_open(self->yajl);
	
	while (k = PNode_key(self))
	{
		//Datum_appendQuoted_(d, k);
		yajl_gen_datum(self->yajl, k);

		PQuery_enumerate(q);
		//if (PNode_key(self)) Datum_appendCString_(d, ",");
	}

	yajl_gen_array_close(self->yajl);
	Datum_appendYajl_(d, self->yajl);
	//Datum_appendCString_(d, "]");
	return 0;
}

int PNode_op_pairs(PNode *self, Datum *d)
{	
	PQuery *q = PNode_startQuery(self);
	PNode *tmpNode = PDB_allocNode(self->pdb);
	Datum *k;
	
	//Datum_appendCString_(d, "[");
	yajl_gen_array_open(self->yajl);
	
	while (k = PNode_key(self))
	{		
		yajl_gen_array_open(self->yajl);
		//Datum_appendCString_(d, "[");
		//Datum_appendQuoted_(d, k);
		//Datum_appendCString_(d, ",");
		yajl_gen_datum(self->yajl, k);
					
		if(!Datum_beginsWithCString_(k, "_"))
		{
			PNode_setPid_(tmpNode, PNode_value(self));
			PNode_op_object(tmpNode, d);
		}
		else
		{
			//Datum_appendQuoted_(d, PNode_value(self));
			yajl_gen_datum(self->yajl, PNode_value(self));
		}
				
		PQuery_enumerate(q);
		//Datum_appendCString_(d, "]");
		//if (PNode_key(self)) Datum_appendCString_(d, ",");
		yajl_gen_array_close(self->yajl);
	}
	
	//Datum_appendCString_(d, "]");
	yajl_gen_array_close(self->yajl);
	Datum_appendYajl_(d, self->yajl);
	return 0; 
}

/*
int PNode_op_pairs(PNode *self, Datum *d)
{	
	PQuery *q = PNode_startQuery(self);
	PNode *tmpNode = PDB_allocNode(self->pdb);
	Datum *k;
	
	Datum_appendCString_(d, "{");
	
	while (k = PNode_key(self))
	{		
		Datum_appendQuoted_(d, k);
		Datum_appendCString_(d, ":");
					
		if(!Datum_beginsWithCString_(k, "_"))
		{
			PNode_setPid_(tmpNode, PNode_value(self));
			PNode_op_json(tmpNode, d);
		}
		else
		{
			Datum_appendQuoted_(d, PNode_value(self));
		}
				
		PQuery_enumerate(q);
		if (PNode_key(self)) Datum_appendCString_(d, ",");
	}
	
	Datum_appendCString_(d, "}");
	return 0; 
}
*/

int PNode_op_values(PNode *self, Datum *d)
{	
	PQuery *q = PNode_startQuery(self);
	PNode *tmpNode = PDB_allocNode(self->pdb);
	Datum *k;
	
	//Datum_appendCString_(d, "[");
	yajl_gen_array_open(self->yajl);
	
	while (k = PNode_key(self))
	{		
		//Datum_appendQuoted_(d, k);
		//Datum_appendCString_(d, ":");
					
		if(!Datum_beginsWithCString_(k, "_"))
		{
			PNode_setPid_(tmpNode, PNode_value(self));
			PNode_op_object(tmpNode, d);
		}
		else
		{
			//Datum_appendQuoted_(d, PNode_value(self));
			yajl_gen_datum(self->yajl, PNode_value(self));
		}
				
		PQuery_enumerate(q);
		//if (PNode_key(self)) Datum_appendCString_(d, ",");
	}
	
	//Datum_appendCString_(d, "]");
	yajl_gen_array_close(self->yajl);
	return 0; 
}

/*
int PNode_op_rm(PNode *self, Datum *d)
{	
	Datum *k;
	long size = PNode_size(self);
	long removeCount = 0;
	
	while ((k = PNode_key(self)))
	{
		PDB_willWrite(self->pdb);
		if(!tcbdbcurout(self->cursor)) break;
		removeCount ++;
		if (Datum_equals_(k, endKey)) break;
	}
	
	PNode_setSize_(self, size - removeCount);
	
	Datum_appendLong_(d, count);
	return 0;
}
*/

int PNode_op_rm(PNode *self, Datum *d)
{
	PQuery *q = PNode_startQuery(self);
	Datum *k;
	long size = PNode_size(self);
	long removeCount = 0;
	
	if(!q) PNode_first(self);
	
	while (k = PNode_key(self))
	{
		PDB_willWrite(self->pdb);
		if(!tcbdbcurout(self->cursor)) break;
		
		// hack to avoid skipping a step by backing up before calling PQuery_enumerate
		if(PQuery_stepDirection(q) == 1)
		{
			tcbdbcurprev(self->cursor);
		}
		else 
		{
			tcbdbcurnext(self->cursor);
		}

		removeCount ++;
		//count += PNode_removeAt_(self, k);
		PQuery_enumerate(q);
	}

	PNode_setSize_(self, size - removeCount);
	
	
	yajl_gen_integer(self->yajl, removeCount);
	Datum_appendYajl_(d, self->yajl);
	//Datum_appendLong_(d, removeCount);
	return 0;  
}

// meta slots ----------------------------------------------------

Datum *PNode_metaSlotFor_(PNode *self, Datum *key)
{
	Datum *d = PDB_allocDatum(self->pdb);
	Datum_copy_(d, self->pid);
	Datum_appendCString_(d, "/m/");
	Datum_append_(d, key);
	return d;
}

int PNode_metaAt_put_(PNode *self, Datum *key, Datum *value)
{
	Datum *slot = PNode_metaSlotFor_(self, key);
	
	if(!PDB_at_put_(self->pdb, Datum_data(slot), (int)Datum_size(slot), Datum_data(value), (int)Datum_size(value)))
	{
		return -1;
	}
	
	return 0;
}	

Datum *PNode_metaAt_(PNode *self, Datum *d)
{
	Datum *slot = PNode_metaSlotFor_(self, d);
	
	{
		int vSize;
		void *v = PDB_at_(self->pdb, Datum_data(slot), (int)Datum_size(slot), &vSize);
		
		if (v)
		{
			Datum *value = PDB_allocDatum(self->pdb);
			Datum_setData_size_(value, v, vSize);
			return value;
		}
	}
	
	return 0x0;
}	

// get permisions ----------------------------------------------------

Datum *PNode_owner(PNode *self)
{
	DATUM_STACKALLOCATED(key, "owner");
	return PNode_metaAt_(self, key);
}

int PNode_setOwner_(PNode *self, Datum *owner)
{
	DATUM_STACKALLOCATED(key, "owner");
	return PNode_metaAt_put_(self, key, owner);
}

int PNode_boolMetaAtCString_(PNode *self, char *k)
{
	DATUM_STACKALLOCATED(key, k);
	Datum *v = PNode_metaAt_(self, key);
	return (!v || !Datum_equalsCString_(v, "n"));
}

// set permisions ----------------------------------------------------

int PNode_metaAtString_putBool_(PNode *self, char *k, int b)
{
	char *v = b ? "y" : "n";

	DATUM_STACKALLOCATED(key, k);
	DATUM_STACKALLOCATED(value, v);
	PNode_metaAt_put_(self, key, value);
	
	return 0;
}

int PNode_isPublicReadable(PNode *self)
{
	return PNode_boolMetaAtCString_(self, "publicReadable");
}

int PNode_isPublicWritable(PNode *self)
{
	return PNode_boolMetaAtCString_(self, "publicWritable");
}

int PNode_isPublicAppendable(PNode *self)
{
	return PNode_boolMetaAtCString_(self, "publicAppendable");
}

int PNode_setIsPublicReadable_(PNode *self, int b)
{
	return PNode_metaAtString_putBool_(self, "publicReadable", b);
}

int PNode_setIsPublicWritable_(PNode *self, int b)
{
	return PNode_metaAtString_putBool_(self, "publicWritable", b);
}

int PNode_setIsPublicAppendable_(PNode *self, int b)
{
	return PNode_metaAtString_putBool_(self, "publicAppendable", b);
}

// check permisions ----------------------------------------------------

int PNode_hasOwner_(PNode *self, Datum *user)
{
	Datum *owner = PNode_owner(self);
	return (owner && Datum_equals_(owner, user));
}

int PNode_readableByUser_(PNode *self, Datum *user)
{	
	return PNode_hasOwner_(self, user) || PNode_isPublicReadable(self);
}

int PNode_writableByUser_(PNode *self, Datum *user)
{	
	return PNode_hasOwner_(self, user) || PNode_isPublicWritable(self);
}

int PNode_appendableByUser_(PNode *self, Datum *user)
{	
	return PNode_hasOwner_(self, user) || PNode_isPublicAppendable(self);
}



