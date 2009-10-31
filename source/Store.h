#ifndef Store_DEFINED
#define Store_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include "StoreCursor.h"

//typedef void  (StoreRequestCallback)(void *);
//typedef void  (StoreWriteMethod)(void *);

typedef struct
{
	/*
	StoreWriteMethod *write;
	StoreWriteMethod *readQuery;
	StoreWriteMethod *readPost;
	StoreWriteMethod *readPost;
	*/
	char *path;
	void *compareFunc;
	void *db;
} Store;

Store *Store_new(void);
void Store_free(Store *self);

void Store_setCompareFunction_(Store *self, void *func);

void Store_setPath_(Store *self, const char *p);
const char *Store_path(Store *self);

int Store_open(Store *self);
int Store_close(Store *self);
int Store_backup(Store *self, const char *backupPath);

const char *Store_error(Store *self);

char *Store_read(Store *self, const char *k, size_t ksize, int *vsize);
int Store_write(Store *self, const char *k, size_t ksize, const char *v, size_t vsize);
int Store_append(Store *self, const char *k, size_t ksize, const char *v, size_t vsize);
int Store_remove(Store *self, const char *k, size_t ksize);
int Store_sync(Store *self);

int Store_begin(Store *self);
int Store_abort(Store *self);
int Store_commit(Store *self);

int Store_size(Store *self);

//StoreCursor *Store_newCursor(Store *self);

#ifdef __cplusplus
}
#endif
#endif
