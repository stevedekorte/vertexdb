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

Log *Log_sharedLog(void);
const char *Log_TimeString(void);
void Log_setPath_(const char *path);
const char *Log_path(void);
int Log_close(void);
int Log_open(void);

#define Log_Printf(s) fputs(Log_TimeString(), Log_sharedLog()->file); fprintf(Log_sharedLog()->file, ": "); fprintf(Log_sharedLog()->file, s); fflush(Log_sharedLog()->file);
#define Log_Printf_(s, a1) fputs(Log_TimeString(), Log_sharedLog()->file); fprintf(Log_sharedLog()->file, ": "); fprintf(Log_sharedLog()->file, s, a1); fflush(Log_sharedLog()->file);
#define Log_Printf__(s, a1, a2) fputs(Log_TimeString(), Log_sharedLog()->file); fprintf(Log_sharedLog()->file, ": "); fprintf(Log_sharedLog()->file, s, a1, a2); fflush(Log_sharedLog()->file);
#define Log_Printf___(s, a1, a2, a3) fputs(Log_TimeString(), Log_sharedLog()->file); fprintf(Log_sharedLog()->file, ": "); fprintf(Log_sharedLog()->file, s, a1, a2, a3); fflush(Log_sharedLog()->file);

#ifdef __cplusplus
}
#endif
#endif
