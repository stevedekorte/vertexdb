
#ifndef PCollector_DEFINED
#define PCollector_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "PDB.h"

typedef struct
{
	PDB *in;
	PNode *inNode;

	PDB *out;
	PNode *outNode;
	
	CHash *savedPids;
	List *saveQueue;
	size_t markCount;
	double maxStepTime;
	
	int isCollecting;
} PCollector;

PCollector *PCollector_new(void);
void PCollector_free(PCollector *self);
void PCollector_setIn_(PCollector *self, void *pdb);

void PCollector_begin(PCollector *self);
void PCollector_step(PCollector *self);
long PCollector_complete(PCollector *self);
//void PCollector_cancel(PCollector *self);

int PCollector_shouldUpdateKey_(PCollector *self, const char *k, int ksize);
int PCollector_hasSaved_(PCollector *self, long pid);
void PCollector_addToSaveQueue_(PCollector *self, long pid);
void PCollector_markPid_(PCollector *self, long pid);
int PCollector_isCollecting(PCollector *self);

void PCollector_showStatus(PCollector *self);

#ifdef __cplusplus
}
#endif
#endif
