
#ifndef PQuery_DEFINED
#define PQuery_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

//#include "PDB.h"
#include "Datum.h"
#include "CHash.h"

//typedef struct PNode PNode;

typedef struct
{
	void *node;
	void *tmpNode;
		
	Datum *id;
	Datum *after;
	Datum *before;
	Datum *whereKey;
	Datum *whereValue;
	Datum *attribute;
		
	unsigned int selectCount;
	unsigned int selectCountMax;
	unsigned int hasFilter;
	unsigned int hasRange;
	int stepDirection;
	int isDone;
} PQuery;

typedef int (PQueryMethod)(PQuery *);

PQuery *PQuery_new(void);
void PQuery_free(PQuery *self);

void PQuery_setNode_(PQuery *self, void *node);
void PQuery_setTmpNode_(PQuery *self, void *node);
void *PQuery_tmpNode(PQuery *self);

void PQuery_setId_(PQuery *self, Datum *d);
void PQuery_setAfter_(PQuery *self, Datum *d);
void PQuery_setBefore_(PQuery *self, Datum *d);
void PQuery_setWhereKey_(PQuery *self, Datum *d);
void PQuery_setWhereValue_(PQuery *self, Datum *d);
void PQuery_setSelectCountMax_(PQuery *self, unsigned int n);
void PQuery_setAttribute_(PQuery *self, Datum *d);
Datum *PQuery_attribute(PQuery *self);
Datum *PQuery_attributeValue(PQuery *self);

int PQuery_setup(PQuery *self);
Datum *PQuery_key(PQuery *self);
void PQuery_enumerate(PQuery *self);
int PQuery_stepDirection(PQuery *self);

int PQuery_cursorMatches(PQuery *self); // private
int PQuery_moveToNextMatch(PQuery *self); // private
int PQuery_isInRange(PQuery *self); // private

unsigned int PQuery_selectCount(PQuery *self);

#ifdef __cplusplus
}
#endif
#endif
