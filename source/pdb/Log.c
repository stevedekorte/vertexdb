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

static Log *globalLog = 0x0;

Log *Log_sharedLog(void)
{
	if(!globalLog)
	{
		globalLog = calloc(1, sizeof(Log));
		globalLog->file = stderr;
	}
	
	return globalLog;
}

const char *Log_TimeString(void)
{
	time_t t = time(NULL);
	struct tm *tt = localtime(&t); 
	strftime(timeString, 128, "%Y-%m-%d %H:%M:%S", tt);	
	return timeString;
}

void Log_setPath_(const char *path)
{
	Log_sharedLog()->path = path;
}

const char *Log_path(void)
{
	return Log_sharedLog()->path;
}

int Log_close(void)
{
	if(Log_sharedLog()->file && Log_sharedLog()->file != stderr)
	{
		return fclose(Log_sharedLog()->file);
	}
	else
	{
		return 0;
	}
}

int Log_open(void)
{
	const char *path = Log_sharedLog()->path;	
	Log_close();
	
	if(path == 0x0) 
	{
		Log_Printf("Logging to stderr\n");
		return 0;
	}

	{
		FILE *file = fopen(path, "a");
		
		if(file)
		{
			Log_sharedLog()->file = file;
			Log_Printf_("Logging to %s\n", path);
			return 0;
		}
		else
		{
			Log_Printf_("Unable to open log file for writing: %s\n", path);
			return 1;
		}
	}
}
