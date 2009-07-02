
#ifndef File_DEFINED
#define File_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "Datum.h"
#include "../Log.h"

typedef struct
{
	Datum *path;
} File;

File *File_new(void);
File *File_with_(const char *path);
void File_free(File *self);

void File_setPathCString_(File *self, const char *path);
const char *File_pathCString(File *self);

void File_setPath_(File *self, Datum *path);
Datum *File_path(File *self);

void File_appendToPath_(File *self, Datum *path);
void File_appendToPathCString_(File *self, const char *path);

int File_exists(File *self);
int File_create(File *self);
int File_remove(File *self);

int File_moveTo_(File *self, File *other);
int File_copyTo_(File *self, File *other);
int File_symbolicallyLinkTo_(File *self, File *other);

int File_createDirectory(File *self);

#ifdef __cplusplus
}
#endif
#endif
