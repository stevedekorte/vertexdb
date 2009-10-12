#ifndef Log_DEFINED
#define Log_DEFINED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct
{
	const char *path;
	FILE *file;
} Log;

#ifdef LOG_C
Log *globalLog;
#else
extern Log *globalLog;
#endif

const char *Log_TimeString(void);
void Log_init(void);
void Log_setPath_(const char *path);
int Log_close(void);
int Log_open(void);

#define Log_Printf(s) fputs(Log_TimeString(), globalLog->file); fprintf(globalLog->file, ": "); fprintf(globalLog->file, s); fflush(globalLog->file);
#define Log_Printf_(s, a1) fputs(Log_TimeString(), globalLog->file); fprintf(globalLog->file, ": "); fprintf(globalLog->file, s, a1); fflush(globalLog->file);
#define Log_Printf__(s, a1, a2) fputs(Log_TimeString(), globalLog->file); fprintf(globalLog->file, ": "); fprintf(globalLog->file, s, a1, a2); fflush(globalLog->file);
#define Log_Printf___(s, a1, a2, a3) fputs(Log_TimeString(), globalLog->file); fprintf(globalLog->file, ": "); fprintf(globalLog->file, s, a1, a2, a3); fflush(globalLog->file);

#ifdef __cplusplus
}
#endif
#endif
