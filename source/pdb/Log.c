#define LOG_C
#include "Log.h"
#undef LOG_C
/*
#include <sys/types.h>  
#include <sys/time.h>  
#include <stdlib.h>  
#include <string.h>
*/
#include <stdlib.h>
#include <time.h>

static char timeString[1024];

const char *Log_TimeString(void)
{
	time_t t = time(NULL);
	struct tm *tt = localtime(&t); 
	strftime(timeString, 128, "%Y-%m-%d %H:%M:%S", tt);	
	return timeString;
}

void Log_init(void)
{
	globalLog = calloc(1, sizeof(Log));
	globalLog->file = stderr;
}

void Log_setPath_(const char *path)
{
	globalLog->path = path;
}

int Log_close(void)
{
	if(globalLog->file && globalLog->file != stderr)
	{
		return fclose(globalLog->file);
	}
	else
	{
		return 0;
	}
}

int Log_open(void)
{
	FILE *file;
	
	Log_close();
	
	file = fopen(globalLog->path, "a");
	if(file)
	{
		globalLog->file = file;
		return 0;
	}
	else
	{
		return 1;
	}
}
