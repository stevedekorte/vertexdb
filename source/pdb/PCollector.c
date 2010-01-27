
#include "PCollector.h"
#include "Log.h"
#include "Datum.h"
#include "Date.h"
#include "Pointer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


PCollector *PCollector_new(void)
{
	PCollector *self = calloc(1, sizeof(PCollector));
	self->out = PDB_new();
	PDB_setUseBackups_(self->out, 0);
	PDB_setHardSync_(self->out, 0);
	PDB_setPathCString_(self->out, "/tmp/db.tc"); // hack - change to a uniqueID
		
	self->savedPids = CHash_new();
	CHash_setEqualFunc_(self->savedPids, (CHashEqualFunc *)Pointer_equals_);
	CHash_setHash1Func_(self->savedPids, (CHashHashFunc *)Pointer_hash1);
	CHash_setHash2Func_(self->savedPids, (CHashHashFunc *)Pointer_hash2);
	
	self->saveQueue  = List_new();
	self->markCount = 0;
	
	self->maxStepTime = .1;
	return self;
}

void PCollector_putCallback(void *arg, const char *k, int ksize, const char *v, int vsize)
{
	PCollector *self = arg;
	
	if(PCollector_shouldUpdateKey_(self, k, ksize))
	{
		PDB_at_put_(self->out, k, ksize, v, vsize);
		
		// if the slot is to a pointer, make sure the node it points to will get marked
		char *key = strstr(k, "/s/");
		
		if(key)
		{
			if(strlen(key) < 4) return; // shouldn't happen, but just to be safe
			key += strlen("/s/"); // move to the slotName
			
			if(key[0] != '_') // slotName is for a pointer, so mark it
			{
				PCollector_addToSaveQueue_(self, atol(v));
			}
		}
	}

}

void PCollector_catCallback(void *arg, const char *k, int ksize, const char *v, int vsize)
{
	PCollector *self = arg;
	if(PCollector_shouldUpdateKey_(self, k, ksize))
	{
		PDB_at_cat_(self->out, k, ksize, v, vsize);
	}
}

void PCollector_removeCallback(void *arg, const char *k, int ksize)
{
	PCollector *self = arg;
	if(PCollector_shouldUpdateKey_(self, k, ksize))
	{
		PDB_removeAt_(self->out, k, ksize);
	}
}

void PCollector_setIn_(PCollector *self, void *pdb)
{
	self->in = pdb;
	PDB_setDelegate_(self->in, self);
	PDB_setPutCallback_(self->in, (PDBPutCallback *)PCollector_putCallback);
	PDB_setCatCallback_(self->in, (PDBCatCallback *)PCollector_catCallback);
	PDB_setRemoveCallback_(self->in, (PDBRemoveCallback *)PCollector_removeCallback);
}

void PCollector_free(PCollector *self)
{	
	if(self->in) PDB_setDelegate_(self->in, 0x0);
	PDB_free(self->out);
	CHash_free(self->savedPids);
	List_free(self->saveQueue);
	free(self);
}

int PCollector_shouldUpdateKey_(PCollector *self, const char *k, int ksize)
{
	const char *slash = strchr(k, '/');
	
	char pidString[128];
	memcpy(pidString, k, slash - k);
	long pid = atol(pidString);
	
	int hasSaved = PCollector_hasSaved_(self, pid);
	//printf("hasSaved %s %i\n", pidString, hasSaved);
	return hasSaved; //PCollector_hasSaved_(self, pid);
}

// --------------------------------------------

void PCollector_begin(PCollector *self)
{	
	if(self->isCollecting) 
	{
		printf("PCollector_beginCollectGarbage: already in collection - ignored\n");
		return;
	}
	
	self->isCollecting = 1;
	PDB_setDelegate_(self->in, self);

	// in db already open
	self->inNode = PDB_newNode(self->in);

	PDB_remove(self->out);	
	PDB_open(self->out);	
	self->outNode = PDB_newNode(self->out);
		
	Log_Printf__("PCollector: beginCollectGarbage %iMB %i records before collect\n", 
		(int)PDB_sizeInMB(self->in),
		(int)PDB_numberOfKeys(self->in)
	);

	self->beginKeyCount = PDB_numberOfKeys(self->in);
	self->beginTime = time(NULL);
	
	PCollector_addToSaveQueue_(self, 1); // root node
}

void PCollector_step(PCollector *self)
{
	if (!self->isCollecting) return;
	
	double t1 = Date_SecondsFrom1970ToNow();
	int count = 0;
	double dt = 0;
	
	while (dt < self->maxStepTime)
	{
		long pid = (long)List_pop(self->saveQueue);
		PCollector_markPid_(self, pid);
		count ++;
		
		if (List_size(self->saveQueue) == 0)
		{
			//PDB_commit(self->out);
			PCollector_complete(self);
			break;
		}
		
		dt = Date_SecondsFrom1970ToNow() - t1;
	}
	//PDB_commit(self->out);
}

long PCollector_complete(PCollector *self)
{
	CHash_clear(self->savedPids);
	
	PNode_free(self->inNode);
	self->inNode = 0x0;
	PDB_close(self->in);

	PNode_free(self->outNode);
	self->outNode = 0x0;
	PDB_close(self->out);
	
	PDB_moveTo_(self->out, self->in);
	PDB_open(self->in);
	PDB_setDelegate_(self->in, 0x0);
	
	//File_remove(self->dbFile);
	//File_moveTo_(out->dbFile, self->dbFile);
		
	Log_Printf__("PCollector: completeCollectGarbage %iMB and %i records after collect\n", 
		(int)PDB_sizeInMB(self->in),
		(int)PDB_numberOfKeys(self->in)
	);
	
	Log_Printf_("PCollector: collected %i keys\n",  (int)(self->beginKeyCount-PDB_numberOfKeys(self->in)));
	Log_Printf_("PCollector: took %i minutes\n",  (int)difftime(time(NULL), self->beginTime)/60);
	
	self->isCollecting = 0;
	return 0;
}

void PCollector_showStatus(PCollector *self)
{
	Log_Printf___(" collector queued:%i saved:%i savedPids:%0.2fM\n", 
		(int)List_size(self->saveQueue),	
		(int)self->markCount, 
		((float)self->savedPids->size)/1000000.0
	);
}

void PCollector_markPid_(PCollector *self, long pid)
{
	assert(CHash_at_(self->savedPids, (void *)pid) == (void *)0x1);

	PNode_setPidLong_(self->inNode, pid);
	PNode_setPidLong_(self->outNode, pid);

	Datum *k;
	PNode_first(self->inNode);

	while ((k = PNode_key(self->inNode)))
	{
		Datum *v = PNode_value(self->inNode);
		if (Datum_data(k)[0] != '_')
		{
			long pid = Datum_asLong(v);

			if (pid)
			{
				PCollector_addToSaveQueue_(self, pid);
			}
		}
		PNode_atPut_(self->outNode, k, v);
		//PDB_commit(self->out);
		PNode_next(self->inNode);
		Datum_poolFreeRefs();
	}


	self->markCount ++;
	//PDB_commit(self->out);
	//if (self->markCount % 10 == 0) 
	{ 
		//PDB_commit(self->out);
	}
	if (self->markCount % 10000 == 0) 
	{
		PDB_commit(self->out);
		PCollector_showStatus(self);
	}
}

int PCollector_isCollecting(PCollector *self)
{
	return self->isCollecting;
}

int PCollector_hasSaved_(PCollector *self, long pid)
{
	return CHash_at_(self->savedPids, (void *)pid) != 0x0;
}

void PCollector_addToSaveQueue_(PCollector *self, long pid)
{
	if (!PCollector_hasSaved_(self, pid))
	{
		List_append_(self->saveQueue, (void *)pid);
		CHash_at_put_(self->savedPids, (void *)pid, (void *)0x1);
	}
}
