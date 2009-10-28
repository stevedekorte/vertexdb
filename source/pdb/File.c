#include "File.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int File_System(Datum *cmd)
{
	//Log_Printf_("system: %s\n", Datum_data(cmd));
	int result = system(Datum_data(cmd));
	return result;
}

File *File_new(void)
{
	File *self = calloc(1, sizeof(File));
	self->path = Datum_new();
	return self;
}

File *File_with_(const char *path)
{
	File *aFile = File_new();
	File_setPathCString_(aFile, path);
	return aFile;
}

void File_free(File *self)
{
	Datum_free(self->path);
	free(self);
}

void File_setPathCString_(File *self, const char *path)
{
	Datum_setCString_(self->path, path);
}

const char *File_pathCString(File *self)
{
	return Datum_data(self->path);
}

void File_setPath_(File *self, Datum *path)
{
	Datum_copy_(self->path, path);
}

Datum *File_path(File *self)
{
	return self->path;
}

void File_appendToPath_(File *self, Datum *path)
{
	Datum_append_(self->path, path);
}

void File_appendToPathCString_(File *self, const char *path)
{
	Datum_appendCString_(self->path, path);
}


void File_fatalError_(File *self, char *s)
{
	//Log_Printf__("File error: %s\n", s);
	exit(-1);
}

int File_exists(File *self)
{
	FILE *fp = fopen(Datum_data(self->path), "r");
	if (fp) { fclose(fp); return 1; }
	return 0;
}
	
int File_create(File *self)
{
	FILE *fp = fopen(Datum_data(self->path), "w");
	
	if(!fp)
	{
		Log_Printf_("File: unable to create file '%s'\n", Datum_data(self->path));
		return -1;
	}
	
	fclose(fp);
	
	return 0;
}

int File_remove(File *self)
{
	int result;
	Datum *cmd = Datum_newWithCString_("rm ");
	Datum_append_(cmd, self->path);
	result = File_System(cmd);
	Datum_free(cmd);
	return result;
}

int File_moveTo_(File *self, File *other)
{
	int result;
	Datum *cmd = Datum_newWithCString_("mv ");
	Datum_append_(cmd, self->path);
	Datum_appendCString_(cmd, " ");
	Datum_append_(cmd, File_path(other));
	result = File_System(cmd);
	Datum_free(cmd);
	return result;
}

int File_copyTo_(File *self, File *other)
{
	int result;
	Datum *cmd = Datum_newWithCString_("cp ");
	Datum_append_(cmd, self->path);
	Datum_appendCString_(cmd, " ");
	Datum_append_(cmd, File_path(other));
	result = File_System(cmd);
	Datum_free(cmd);
	return result;
}

int File_symbolicallyLinkTo_(File *self, File *other)
{
	int result;
	Datum *cmd = Datum_newWithCString_("ln -sf ");
	Datum_append_(cmd, self->path);
	Datum_appendCString_(cmd, " ");
	Datum_append_(cmd, File_path(other));
	result = File_System(cmd);
	Datum_free(cmd);
	return result;
}

int File_createDirectory(File *self)
{
	int result;
	Datum *cmd = Datum_newWithCString_("mkdir -p ");
	Datum_append_(cmd, self->path);
	result = File_System(cmd);
	Datum_free(cmd);
	return result;
}
