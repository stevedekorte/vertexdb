#include "PDB.h"
#include "List.h"
#include <err.h>
#include <event.h>
#include <evhttp.h>
#include "RunningStat.h"
#include "CHash.h"
#include "Pool.h"
#include <yajl/yajl_gen.h>

#define HTTP_SERVERERROR 500
#define HTTP_SERVERERROR_MESSAGE "Internal Server Error"
#define HTTP_OK_MESSAGE	"OK"

typedef struct
{
	PDB *pdb;
	Pool *pool;
	
	struct evhttp *httpd;
	struct evhttp_request *request;
	int port;

	int isDaemon;
	const char *logPath;
	const char *pidPath;
	
	Datum *staticPath;
	
	CHash *actions;
	CHash *query;
	CHash *ops;
	
	Datum *emptyDatum;
	Datum *uriPath;
	
	Datum *cookie;
	Datum *userPath;
	Datum *userId;

	Datum *post;
	
	size_t requestCount;
	size_t writesPerCommit;
	time_t lastBackupTime;
	time_t lastStatsUpdateTime;
	
	int shutdown;
	Datum *error;
	Datum *result;
	
	RunningStat *rstat;
	size_t requestsPerSample;
	yajl_gen yajl;
} VertexServer;

typedef int (VertexAction)(VertexServer *);

VertexServer *VertexServer_new(void);
void VertexServer_free(VertexServer *self);

void VertexServer_setPort_(VertexServer *self, int port);
void VertexServer_setDbPath_(VertexServer *self, char *path);
void VertexServer_setLogPath_(VertexServer *self, const char *path);
void VertexServer_setPidPath_(VertexServer *self, const char *path);
void VertexServer_setIsDaemon_(VertexServer *self, int isDaemon);

int VertexServer_process(VertexServer *self);
int VertexServer_run(VertexServer *self);
int VertexServer_shutdown(VertexServer *self);

int VertexServer_api_collectGarbage(VertexServer *self);
int VertexServer_api_shutdown(VertexServer *self);
