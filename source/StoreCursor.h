#ifndef StoreCursor_DEFINED
#define StoreCursor_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "Date.h"

typedef struct
{
	void *store;
	void *cursor;
} StoreCursor;

StoreCursor *StoreCursor_new(void);
void StoreCursor_free(StoreCursor *self);

void StoreCursor_setStore_(StoreCursor *self, void *store);

void StoreCursor_open(StoreCursor *self);
void StoreCursor_close(StoreCursor *self);

int StoreCursor_next(StoreCursor *self);
int StoreCursor_previous(StoreCursor *self);
void StoreCursor_jump(StoreCursor *self, const char *k, size_t ksize);
int StoreCursor_remove(StoreCursor *self);

char *StoreCursor_key(StoreCursor *self, int *size);
char *StoreCursor_value(StoreCursor *self, int *size);

//void StoreCursor_first(StoreCursor *self);
//void StoreCursor_last(StoreCursor *self);
//void StoreCursor_insert(StoreCursor *self);

#ifdef __cplusplus
}
#endif
#endif
