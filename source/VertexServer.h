#include "PDB.h"
#include "List.h"
#include "CHash.h"
#include "Pool.h"
#include "Yajl_extras.h"
#include "HttpServer.h"

typedef struct
{
	// the database
	PDB *pdb;
	
	// the http server
	HttpServer *httpServer;
	HttpRequest *httpRequest;
	HttpResponse *httpResponse;
	Datum *result; // used for composing the response string

	// the global json generator
	yajl_gen yajl;
	
	// hash table of VertextServer action function pointers
	CHash *actions;
	// hash table of PNode op function pointers
	CHash *ops;
	
	// daemon related
	int isDaemon;
	const char *pidPath;
	int debug;
} VertexServer;

typedef int (VertexAction)(VertexServer *);

VertexServer *VertexServer_new(void);
void VertexServer_free(VertexServer *self);

// command line options
void VertexServer_setPort_(VertexServer *self, int port);
void VertexServer_setDbPath_(VertexServer *self, char *path);
void VertexServer_setLogPath_(VertexServer *self, const char *path);
void VertexServer_setPidPath_(VertexServer *self, const char *path);
void VertexServer_setIsDaemon_(VertexServer *self, int isDaemon);
void VertexServer_setDebug_(VertexServer *self, int aBool);
void VertexServer_setHardSync_(VertexServer *self, int aBool);

// request processing
int VertexServer_process(VertexServer *self);
int VertexServer_run(VertexServer *self);
int VertexServer_shutdown(VertexServer *self);
void VertexServer_requestHandler(void *arg);
 
// apis
int VertexServer_api_collectGarbage(VertexServer *self);
int VertexServer_api_shutdown(VertexServer *self);
