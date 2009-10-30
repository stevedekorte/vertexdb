/*
	Notes:
	See http://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html 
	for valid response headers

bool tcbdbput(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);
bool tcbdbputcat(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);
bool tcbdbout(TCBDB *bdb, const void *kbuf, int ksiz);
void *tcbdbget(TCBDB *bdb, const void *kbuf, int ksiz, int *sp);
int tcbdbvsiz(TCBDB *bdb, const void *kbuf, int ksiz);

struct fuse_operations {
    int (*mkdir) (const char *, mode_t);
    int (*rmdir) (const char *);

    int (*symlink) (const char *, const char *);
    int (*link) (const char *, const char *);
    int (*unlink) (const char *);

    int (*rename) (const char *, const char *);

    int (*getattr) (const char *, struct stat *);
    int (*truncate) (const char *, off_t);

    int (*open) (const char *, struct fuse_file_info *);
    int (*read) (const char *, char *, size_t, off_t, struct fuse_file_info *);
    int (*write) (const char *, const char *, size_t, off_t,struct fuse_file_info *);
    int (*release) (const char *, struct fuse_file_info *);
    int (*fsync) (const char *, int, struct fuse_file_info *);
		
    int (*readlink) (const char *, char *, size_t);
    int (*getdir) (const char *, fuse_dirh_t, fuse_dirfil_t);
    int (*mknod) (const char *, mode_t, dev_t);
    int (*chmod) (const char *, mode_t);
    int (*chown) (const char *, uid_t, gid_t);
    int (*utime) (const char *, struct utimbuf *);
    int (*statfs) (const char *, struct statfs *);
    int (*flush) (const char *, struct fuse_file_info *);
	
    int (*setxattr) (const char *, const char *, const char *, size_t, int);
    int (*getxattr) (const char *, const char *, char *, size_t);
    int (*listxattr) (const char *, char *, size_t);
    int (*removexattr) (const char *, const char *);
};
*/

#include "VertexServer.h"
#include "Log.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

#include <err.h>
#include <event.h>
#include <evhttp.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "Date.h"
#include "Socket.h"
#include "PQuery.h"

#define USER_ID_LENGTH 12
//#define COMMIT_PERIODICALLY 1

static VertexServer *globalVertexServer = 0x0;

// ---------------------------------------------------------

void Datum_decodeUri(Datum *self)
{
	char *k = evhttp_decode_uri(Datum_data(self));
	Datum_setCString_(self, k);
	free(k);
}

// ----------------------------------------------------------

void VertexServer_setupYajl(VertexServer *self)
{
	if(self->yajl) 	yajl_gen_free(self->yajl);

	yajl_gen_config config = { 0, "" };
	self->yajl = yajl_gen_alloc(&config, NULL);
	PDB_setYajl_(self->pdb, self->yajl);
}

VertexServer *VertexServer_new(void)
{
	VertexServer *self = calloc(1, sizeof(VertexServer));
	globalVertexServer = self;
	self->port = 8080;
	srand((clock() % INT_MAX));
	
	self->pdb   = PDB_new();
	VertexServer_setupYajl(self);
	
	self->pool  = Pool_new();
	
	self->query = CHash_new();	
	CHash_setEqualFunc_(self->query, (CHashEqualFunc *)Datum_equals_);
	CHash_setHash1Func_(self->query, (CHashHashFunc *)Datum_hash1);
	CHash_setHash2Func_(self->query, (CHashHashFunc *)Datum_hash2);

	self->actions = CHash_new();
	CHash_setEqualFunc_(self->actions, (CHashEqualFunc *)Datum_equals_);
	CHash_setHash1Func_(self->actions, (CHashHashFunc *)Datum_hash1);
	CHash_setHash2Func_(self->actions, (CHashHashFunc *)Datum_hash2);
	
	self->ops = CHash_new();
	CHash_setEqualFunc_(self->ops, (CHashEqualFunc *)Datum_equals_);
	CHash_setHash1Func_(self->ops, (CHashHashFunc *)Datum_hash1);
	CHash_setHash2Func_(self->ops, (CHashHashFunc *)Datum_hash2);

	self->emptyDatum = Datum_new();
	self->uriPath    = Datum_new(); 
	//self->staticPath = Datum_new(); 

	self->cookie     = Datum_new(); 
	self->userPath   = Datum_new(); 
	self->userId     = Datum_new(); 
	
	self->post       = Datum_new(); 
		
	self->writesPerCommit   = 1000;
	self->requestsPerSample = 10000;
	self->rstat = RunningStat_new();
	self->lastBackupTime = time(NULL);

	self->error = Datum_new(); 
	self->result = Datum_new(); 


	return self;
}

void VertexServer_free(VertexServer *self)
{
	PDB_free(self->pdb);
	Pool_free(self->pool);
	
	CHash_free(self->actions);
	CHash_free(self->ops);
	CHash_free(self->query);
	Datum_free(self->emptyDatum);
	Datum_free(self->uriPath);
	//Datum_free(self->staticPath);

	Datum_free(self->cookie);
	Datum_free(self->userPath);
	Datum_free(self->userId);
	Datum_free(self->error);
	
	Datum_free(self->post);
	Datum_free(self->error);
	Datum_free(self->result);
	RunningStat_free(self->rstat);

	if(self->yajl) 	yajl_gen_free(self->yajl);

	free(self);
}

void VertexServer_setError_(VertexServer *self, const char *s)
{
	Datum_setCString_(self->error, s);
}

void VertexServer_appendErrorDatum_(VertexServer *self, Datum *d)
{
	Datum_append_(self->error, d);
}

void VertexServer_setPort_(VertexServer *self, int port)
{
	self->port = port;
}

/*
void VertexServer_setStaticPath_(VertexServer *self, char *path)
{
	Datum_setCString_(self->staticPath, path);
}

void VertexServer_showUri(VertexServer *self)
{
	printf("request: '%s'\n", Datum_data(self->uriPath));
}
*/

void *CHash_atString_(CHash *self, const char *s)
{
	DATUM_STACKALLOCATED(k, s);
	return CHash_at_(self, k);
}

void VertexServer_parseUri_(VertexServer *self, const char *uri)
{
	int index;
	Datum *uriDatum = POOL_ALLOC(self->pool, Datum);
	Datum_setCString_(uriDatum, uri);
	
	if(self->debug) { Log_Printf_("request: %s\n", uri); }
	
	CHash_clear(self->query);
	
	Datum_setSize_(self->uriPath, 0);
	index = Datum_from_beforeChar_into_(uriDatum, 1, '?', self->uriPath);
	
	for (;;)
	{
		Datum *key   = POOL_ALLOC(self->pool, Datum);
		Datum *value = POOL_ALLOC(self->pool, Datum);
		
		index = Datum_from_beforeChar_into_(uriDatum, index + 1, '=', key);
		Datum_decodeUri(key);
		Datum_nullTerminate(key);
		if (Datum_size(key) == 0) break;
		
		index = Datum_from_beforeChar_into_(uriDatum, index + 1, '&', value);
		Datum_decodeUri(value);
		Datum_nullTerminate(value);

		CHash_at_put_(self->query, key, value);
		if (Datum_size(value) == 0) break;
	}	
}

Datum *VertexServer_queryValue_(VertexServer *self, const char *key)
{
	Datum *value = CHash_atString_(self->query, key);
	return value ? value : self->emptyDatum;
}

void VertexServer_vendCookie(VertexServer *self)
{
	Datum_setCString_(self->cookie, "userId=");
	Datum_append_(self->cookie, self->userId);
	Datum_appendCString_(self->cookie, "; expires=Fri, 31-Dec-2200 12:00:00 GMT; path=/;");
	evhttp_add_header(self->request->output_headers, "Set-Cookie", Datum_data(self->cookie));
}

// --- API -------------------------------------------------------------

int VertexServer_api_setCursorPathOnNode_(VertexServer *self, PNode *node)
{	
	int r = PNode_moveToPathIfExists_(node, self->uriPath);
	
	if (r)
	{
		VertexServer_setError_(self, "path does not exist: ");
		VertexServer_appendErrorDatum_(self, self->uriPath);
	}
	
	return r;
}

// ------------------------------------

int VertexServer_api_size(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	//Datum *value = VertexServer_queryValue_(self, "mode");

	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	
	Datum_appendLong_(self->result, (long)PNode_size(node));

	return 0;
}

int VertexServer_api_chmod(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	//Datum *value = VertexServer_queryValue_(self, "mode");

	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	
	//PNode_atPut_(node, key, value);

	return 0;
}

int VertexServer_api_chown(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	//Datum *owner = VertexServer_queryValue_(self, "owner");
	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	
	/*
	if(!PNode_hasOwner_(node, VertexServer_user(self))
	{
		Datum_appendCString_(self->error, "only the owner can change this node's owner");
		return -1;
	}
	*/
	
	//PNode_setOwner_(node, owner);

	return 0;
}

void VertexServer_setupPQuery_(VertexServer *self, PQuery *q)
{
	PQuery_setId_(q, VertexServer_queryValue_(self, "id"));
	PQuery_setAfter_(q, VertexServer_queryValue_(self, "after"));
	PQuery_setBefore_(q, VertexServer_queryValue_(self, "before"));
	PQuery_setSelectCountMax_(q, Datum_asInt(VertexServer_queryValue_(self, "count")));
	PQuery_setWhereKey_(q, VertexServer_queryValue_(self, "whereKey"));
	PQuery_setWhereValue_(q, VertexServer_queryValue_(self, "whereValue"));
	PQuery_setAttribute_(q, VertexServer_queryValue_(self, "attribute"));
	//PQuery_setMode_(q, VertexServer_queryValue_(self, "mode"));
}

int VertexServer_api_select(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	PQuery *q = PNode_query(node);
	VertexServer_setupPQuery_(self, q);
	Datum *opName;
	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	
	opName = (Datum *)CHash_atString_(self->query, "op");

	if (opName)
	{ 
		PNodeOp *op = (PNodeOp *)CHash_at_(self->ops, opName);
		
		if (op)
		{
			return op(node, self->result);
		}
	}

	Datum_appendCString_(self->error, "invalid node op");
	
	return -1;
}

int VertexServer_api_show(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	//Datum *value = VertexServer_queryValue_(self, "mode");

	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	
	Datum_appendLong_(self->result, (long)PNode_size(node));

	return 0;
}

int VertexServer_api_rm(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	Datum *key = VertexServer_queryValue_(self, "key");	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 0; // ignore if it doesn't exist
	PNode_removeAt_(node, key);
	return 0;
}

int VertexServer_api_read(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	Datum *key = VertexServer_queryValue_(self, "key");	
	Datum *value;
	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;

	value = PNode_at_(node, key);

	if (value) 
	{
		yajl_gen_datum(self->yajl, value);
	}
	else
	{
		yajl_gen_null(self->yajl);
	}
	
	Datum_appendYajl_(self->result, self->yajl);
	return 0;
}

int VertexServer_api_write(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	Datum *mode  = VertexServer_queryValue_(self, "mode");
	Datum *key   = VertexServer_queryValue_(self, "key");
	Datum *value = VertexServer_queryValue_(self, "value");
	
	/*
	if(Datum_size(value) == 0)
	{
		value = self->post;
		//VertexServer_setError_(self, "empty keys not accepted");
		//return -1;
	}
	*/
	
	/*
	if(Datum_size(value) == 0)
	{
		VertexServer_setError_(self, "empty values not accepted");
		return -1;
	}
	*/

	if (PNode_moveToPathIfExists_(node, self->uriPath) != 0) 
	{
		VertexServer_setError_(self, "write path does not exist: ");
		VertexServer_appendErrorDatum_(self, self->uriPath);
		return -1;
	}	
	
	if(Datum_equalsCString_(mode, "append"))
	{
		PNode_atCat_(node, key, value);
	}
	else
	{
		PNode_atPut_(node, key, value);
	}
	
	return 0;
}

int VertexServer_api_link(VertexServer *self)
{
	PNode *toNode   = PDB_allocNode(self->pdb);
	PNode *fromNode = PDB_allocNode(self->pdb);
	
	Datum *key      = VertexServer_queryValue_(self, "key");
	Datum *fromPath = self->uriPath;
	Datum *toPath   = VertexServer_queryValue_(self, "toPath");

	if (PNode_moveToPathIfExists_(toNode, toPath) != 0) 
	{
		VertexServer_setError_(self, "to path does not exist: ");
		VertexServer_appendErrorDatum_(self, toPath);
		return -1;
	}
		
	if (PNode_moveToPathIfExists_(fromNode, fromPath) != 0) 
	{
		VertexServer_setError_(self, "from path does not exist: ");
		VertexServer_appendErrorDatum_(self, fromPath);
		return -1;
	}	

	PNode_atPut_(toNode, key, PNode_pid(fromNode));

	return 0;
}

int VertexServer_api_transaction(VertexServer *self)
{
	Datum *uri = Datum_new();
	int error = 0;
	int r, result;
	
	PDB_commit(self->pdb);
	
	do
	{
		Datum_copy_(uri, self->post);
		r = Datum_sepOnChars_with_(uri, "\n", self->post);

		if (Datum_size(uri) == 0) 
		{
			VertexServer_setError_(self, "empty line in transaction");
			error = 1;
			break;
		}
		
		VertexServer_parseUri_(self, Datum_data(uri));
		error = VertexServer_process(self);
		Pool_freeRefs(self->pool);
	} while ((r != -1) && (!error));
	
	if (error)
	{
		PDB_abort(self->pdb);
		result = -1;
	}
	else
	{
		PDB_commit(self->pdb);
		result = 0;
	}
		
	Datum_free(uri);
	return result;
}

int VertexServer_api_mkdir(VertexServer *self)
{
	PNode_moveToPath_(PDB_allocNode(self->pdb), self->uriPath);
	return 0;
}

int VertexServer_api_queuePopTo(VertexServer *self)
{
	PNode *fromNode = PDB_allocNode(self->pdb);
	PNode *toNode   = PDB_allocNode(self->pdb);
	Datum *toPath   = VertexServer_queryValue_(self, "toPath");
	
	long ttl = Datum_asLong(VertexServer_queryValue_(self, "ttl"));
	
	if (PNode_moveToPathIfExists_(fromNode, self->uriPath) != 0) 
	{
		VertexServer_setError_(self, "from path does not exist: ");
		VertexServer_appendErrorDatum_(self, self->uriPath);
		return -1;
	}

	PNode_moveToPath_(toNode, toPath);
	
	//printf("to   pid: %s\n", Datum_data(PNode_pid(toNode)));
	//printf("from pid: %s\n", Datum_data(PNode_pid(fromNode)));
	
	{
		PQuery *q = PNode_query(fromNode);
		VertexServer_setupPQuery_(self, q);
		PNode_startQuery(fromNode);
	
		Datum *k = PQuery_key(q);
		Datum *v = PNode_value(fromNode);
		
		if (k)
		{
			PNode_atPut_(toNode, k, v);
			PNode_moveToKey_(toNode, k);

			// insert queue time
			{
				long now = time(NULL);
				
				Datum *timeKey   = Datum_newWithCString_("_qtime");
				Datum *timeValue = Datum_new();
				Datum_fromLong_(timeValue, now);
				PNode_atPut_(toNode, timeKey, timeValue);
				
				Datum_setCString_(timeKey, "_qexpire");
				Datum_fromLong_(timeValue, now + (ttl == 0 ? 3600 : ttl));
				PNode_atPut_(toNode, timeKey, timeValue);

				Datum_free(timeKey);
				Datum_free(timeValue);
			}
			
			//printf("queueing key %s\n", Datum_data(k));
			yajl_gen_datum(self->yajl, k);
			PNode_removeAt_(fromNode, k);
		}
		else
		{
			yajl_gen_null(self->yajl);
		}
	}
	
	Datum_appendYajl_(self->result, self->yajl);
	
	return 0;
}

int VertexServer_api_queueExpireTo(VertexServer *self) 
{
	PNode *fromNode = PDB_allocNode(self->pdb);
	PNode *toNode   = PDB_allocNode(self->pdb);
	PNode *itemNode = PDB_allocNode(self->pdb);
	Datum *toPath = VertexServer_queryValue_(self, "toPath");
	unsigned int itemsExpired = 0;
	
	if (PNode_moveToPathIfExists_(fromNode, self->uriPath) != 0) 
	{
		VertexServer_setError_(self, "from path does not exist: ");
		VertexServer_appendErrorDatum_(self, self->uriPath);
		return -1;
	}
	
	PNode_moveToPath_(toNode, toPath);
	
	PNode_first(fromNode);
	
	{
		Datum *qTimeKey = Datum_newWithCString_("_qtime");
		Datum *k;
		Datum *qExpireKey   = Datum_newWithCString_("_qexpire");
		long now = time(NULL);
		
		while (k = PNode_key(fromNode))
		{
			Datum *pid = PNode_value(fromNode);
			Datum *qExpireValue;
			
			PNode_setPid_(itemNode, pid);
			qExpireValue = PNode_at_(itemNode, qExpireKey);
			
			if(!qExpireValue)
			{
				Log_Printf("WARNING: expiring a node with no _qexpire value\n");
			}
			
			if(qExpireValue == 0x0 || Datum_asLong(qExpireValue) < now)
			{
				PNode_removeAt_(itemNode, qTimeKey);
				PNode_removeAt_(itemNode, qExpireKey);
				PNode_atPut_(toNode, k, pid);
				PNode_removeAtCursor(fromNode); // the remove will go to the next item
				//PNode_next(fromNode);
				//PNode_removeAt_(fromNode, k);
				itemsExpired ++;
			}
			else
			{
				PNode_next(fromNode);
			}
		}
	
		Datum_free(qTimeKey);
		Datum_free(qExpireKey);
	}
	
	yajl_gen_integer(self->yajl, (long)itemsExpired);
	Datum_appendYajl_(self->result, self->yajl);
	
	return 0;
}

int VertexServer_api_newUser(VertexServer *self)
{	
	PNode *userNode = PDB_allocNode(self->pdb);
	PNode *node = PDB_allocNode(self->pdb);
	
	PNode_moveToPathCString_(node, "users");
	
	do
	{
		Datum_makePidOfLength_(self->userId, USER_ID_LENGTH);
	} while (PNode_at_(node, self->userId));
	
	PNode_create(userNode);
	PNode_atPut_(node, self->userId, PNode_pid(userNode));
		
	VertexServer_vendCookie(self);
	Datum_append_(self->result, self->userId);

	return 0;
}

int VertexServer_api_login(VertexServer *self)
{
	Datum *email = VertexServer_queryValue_(self, "email");
	Datum *inputPassword = VertexServer_queryValue_(self, "password");
	Datum *v;
	PNode *node = PDB_allocNode(self->pdb);
	
	PNode_moveToPathCString_(node, "indexes/user/email");
	v = PNode_at_(node, email);
	
	if (!v)
	{
		VertexServer_setError_(self, "unknown user email address");
		VertexServer_appendErrorDatum_(self, email);
		return 2;
	}
	
	Datum_copy_(self->userId, v);
	Datum_setCString_(self->userPath, "users/");
	Datum_append_(self->userPath, self->userId);
	PNode_moveToPathIfExists_(node, self->userPath);
	{
		Datum *password = PNode_atCString_(node, "_password");
		
		if (!password || !Datum_equals_(password, inputPassword))
		{
			VertexServer_setError_(self, "wrong password");
			return 2; // why 2?
		}
	}
	
	VertexServer_vendCookie(self);
	return 0;
}

int VertexServer_backup(VertexServer *self)
{
	time_t now = time(NULL);
	int result;
	
	Log_Printf_("backup at %i bytes written... ", (int)PDB_bytesWaitingToCommit(self->pdb));
	
	PDB_commit(self->pdb);
	
	result = PDB_backup(self->pdb);

	Log_Printf__("backup %s and took %i seconds\n", 
		result ? "failed" : "successful", (int)difftime(time(NULL), now));
				
	self->lastBackupTime = now;

	return result;
}

void VertexServer_backupIfNeeded(VertexServer *self)
{
	if (self->requestCount % 10000 == 0) // so we don't call time() on every request
	{
		time_t now = time(NULL);
		double dt = difftime(now, self->lastBackupTime);
		
		//Log_Printf_("%i seconds since last backup\n", (int)dt);
		
		if (dt > 60*60*12) // 12 hours
		{
			Log_Printf("starting timed backup\n");
			VertexServer_backup(self);
		}
	}
}

void VertexServer_commitIfNeeded(VertexServer *self)
{
	size_t writeByteCount = PDB_bytesWaitingToCommit(self->pdb);

	if (writeByteCount > 1024*1024*10)
	{
		PDB_commit(self->pdb);
		Log_Printf_("commit at %i bytes written\n", 
			(int)writeByteCount);
	}
}

int VertexServer_api_backup(VertexServer *self)
{
	int result = VertexServer_backup(self);

	if(result)
	{
		Datum_appendCString_(self->result, "backup failed");
	}
	else
	{
		Datum_appendCString_(self->result, "backup successful");
	}

	return result;
	// move to this once backups are integrated with gc
	//return VertexServer_VertexServer_api_collectGarbage(self);
}

int VertexServer_api_collectGarbage(VertexServer *self)
{
	time_t t1 = time(NULL);
	long savedCount = PDB_collectGarbage(self->pdb);
	double dt = difftime(time(NULL), t1);
	
	yajl_gen_map_open(self->yajl);
	yajl_gen_cstring(self->yajl, "saved");
	yajl_gen_integer(self->yajl, savedCount);
	yajl_gen_cstring(self->yajl, "seconds");
	yajl_gen_integer(self->yajl, (int)dt);
	yajl_gen_map_close(self->yajl);
	
	Datum_appendYajl_(self->result, self->yajl);
	Log_Printf_("gc: %s\n", Datum_data(self->result));

	return 0;
}

int VertexServer_api_showStats(VertexServer *self)
{
	/*
	evbuffer_add_printf(self->buf, "runTimeInSeconds: %i\n", (int)(Date_secondsSinceNow(self->rstat->startDate)));
	evbuffer_add_printf(self->buf, "requestsSinceStartup: %i\n", self->requestCount);
	evbuffer_add_printf(self->buf, "aveMicrosecondsPerRequest: %i\n", (int)(1000000.0*RunningStat_aveTime(self->rstat)));
	evbuffer_add_printf(self->buf, "samplesAveragedOver: %i\n", RunningStat_sampleCount(self->rstat) );
	evbuffer_add_printf(self->buf, "requestPerSample: %i\n", self->requestsPerSample);
	*/
	return 0;
}

int VertexServer_api_syncSizes(VertexServer *self)
{
	PDB_syncSizes(self->pdb);
	return 0;
}

int VertexServer_api_sync(VertexServer *self)
{
	PDB_sync(self->pdb);
	return 0;
}

int VertexServer_api_log(VertexServer *self)
{
	Log_Printf("LOG -----------------------------\n\n");
	printf("%s\n", Datum_data(self->post));
	printf("\n---------------------------------\n");
	return 0;
}

int VertexServer_api_view(VertexServer *self)
{
	Datum *before = VertexServer_queryValue_(self, "before");
	Datum *after = VertexServer_queryValue_(self, "after");
	PNode *node = PDB_allocNode(self->pdb);
	Datum *d = self->result;
	int maxCount = 200;
	
	if (PNode_moveToPathIfExists_(node, self->uriPath) != 0) 
	{
		VertexServer_setError_(self, "path does not exist: ");
		VertexServer_appendErrorDatum_(self, self->uriPath);
		return -1;
	}
		Datum_appendCString_(d, "<html>");
		Datum_appendCString_(d, "<title>");
		Datum_append_(d, self->uriPath);
		Datum_appendCString_(d, "</title>");
		Datum_appendCString_(d, "<style>");
		Datum_appendCString_(d, "body { font-family: sans; margin-top:2em; margin-left:2em; }");
		Datum_appendCString_(d, ".path { font-size: 1em; font-weight: normal; font-family: sans; }");
		Datum_appendCString_(d, ".note { color:#aaaaaa; font-size: 1em; font-weight: normal; font-family: sans;  }");
		Datum_appendCString_(d, ".key { color:#000000;  }");
		Datum_appendCString_(d, ".value { color:#888888; white-space:pre; }");
		Datum_appendCString_(d, "a { color: #0000aa; text-decoration: none;  }");

		Datum_appendCString_(d, "</style>\n");
		Datum_appendCString_(d, "</head>\n");
		Datum_appendCString_(d, "<body>\n");
	
	/*
	if(Datum_size(self->uriPath) == 0)
	{
	Datum_appendCString_(d, "/");
	}
	else
	{
	*/
		Datum_appendCString_(d, "<font class=path>");
		Datum_appendCString_(d, "/");
		Datum_append_(d, self->uriPath);
		Datum_appendCString_(d, "</font>");
		Datum_appendCString_(d, "<br>\n");
	//}
	
	PNode_first(node);
	
	if(Datum_size(before))
	{
		PNode_jump_(node, before);
		
		int i;
		for(i = 0; i < maxCount; i++)
		{
			PNode_previous(node);
		}
		PNode_next(node);

	}
	else if(Datum_size(after))
	{
		PNode_jump_(node, after);
	}

	if (Datum_size(after) && PNode_key(node))
	{
		Datum_appendCString_(d, "<a href=/");
		Datum_append_(d, self->uriPath);
		Datum_appendCString_(d, "?before=");
		Datum_append_(d, PNode_key(node));
		Datum_appendCString_(d, ">previous</a>");
	}

	
	Datum_appendCString_(d, "<ul>\n");
	Datum_appendCString_(d, "<table cellpadding=0 cellspacing=0 border=0>");

	{
		Datum *k;
		int count = 0;
		
		while ((k = PNode_key(node)) && count < maxCount)
		{
			Datum_appendCString_(d, "<tr>");
			
			if (Datum_beginsWithCString_(k , "_"))
			{
				Datum_appendCString_(d, "<td align=right style=\"line-height:1.5em\">");
				Datum_appendCString_(d, "<font class=key>");
				Datum_append_(d, k);
				Datum_appendCString_(d, "</font>");
				Datum_appendCString_(d, " ");
				Datum_appendCString_(d, "</td>");
				
				Datum_appendCString_(d, "<td>");
				Datum_appendCString_(d, "&nbsp;&nbsp;<span class=value>");
				Datum_append_(d, PNode_value(node));
				Datum_appendCString_(d, "</span>");
				Datum_appendCString_(d, "<br>\n");
				Datum_appendCString_(d, "</td>");
			}
			else
			{
				
				Datum_appendCString_(d, "<td align=right>");
				Datum_appendCString_(d, "<font class=key>");
				Datum_append_(d, k);
				Datum_appendCString_(d, "</font><br>\n");			
				Datum_appendCString_(d, "</td>");
				
				Datum_appendCString_(d, "<td style=\"line-height:1.5em\">");
				Datum_appendCString_(d, "&nbsp;&nbsp;<a href=");
				if(Datum_size(self->uriPath) != 0) Datum_appendCString_(d, "/");
				Datum_append_(d, self->uriPath);
				Datum_appendCString_(d, "/");
				Datum_append_(d, k);
				Datum_appendCString_(d, "> ↠ ");
				Datum_appendLong_(d, PNode_nodeSizeAtCursor(node));
				Datum_appendCString_(d, "</a> ");
				Datum_appendCString_(d, "<font class=value>");
				Datum_appendCString_(d, "</td>");
			}
			
			Datum_appendCString_(d, "</tr>");
			PNode_next(node);
			count ++;
		}
	}
	Datum_appendCString_(d, "</table>");
	Datum_appendCString_(d, "</ul>\n");
	
	if(PNode_key(node))
	{
		Datum_appendCString_(d, "<a href=/");
		Datum_append_(d, self->uriPath);
		Datum_appendCString_(d, "?after=");
		Datum_append_(d, PNode_key(node));
		Datum_appendCString_(d, ">next</a><br>");
	}
	
	Datum_appendCString_(d, "</body>\n");

	self->isHtml = 1;
	return 0;
}

// ---------------------------------------------------------------------

#define VERTEX_SERVER_ADD_ACTION(name) CHash_at_put_(self->actions, Datum_newWithCString_(#name ""), (void *)VertexServer_api_##name);
#define VERTEX_SERVER_ADD_OP(name) CHash_at_put_(self->ops, Datum_newWithCString_(#name ""), (void *)PNode_op_##name);

void VertexServer_setupActions(VertexServer *self)
{	
/*
	select 
		op: keys / values | pairs / rm | counts | object
		before:id
		after:id
		count:max
		whereKey:k, whereValue:v
	rm
	mkdir
	link
	chmod
	chown
	stat
	size

	read
	write mode: set / append

	queuePopTo
	queueExpireTo

	transaction
	login
	newUser

	shutdown
	backup
	collectGarbage
	stats
*/
	
	// read
	VERTEX_SERVER_ADD_ACTION(select);
	VERTEX_SERVER_ADD_ACTION(read);
	VERTEX_SERVER_ADD_ACTION(size);

	// remove
	VERTEX_SERVER_ADD_ACTION(rm);
			
	// insert
	VERTEX_SERVER_ADD_ACTION(write);
	VERTEX_SERVER_ADD_ACTION(mkdir);
	VERTEX_SERVER_ADD_ACTION(link);
	// rename
	
	// queues
	VERTEX_SERVER_ADD_ACTION(queuePopTo);
	VERTEX_SERVER_ADD_ACTION(queueExpireTo);

	// transaction
	VERTEX_SERVER_ADD_ACTION(transaction);
	
	// users / permissions
	VERTEX_SERVER_ADD_ACTION(login);
	VERTEX_SERVER_ADD_ACTION(newUser);
	
	VERTEX_SERVER_ADD_ACTION(chmod);
	VERTEX_SERVER_ADD_ACTION(chown);

	
	// management
	VERTEX_SERVER_ADD_ACTION(shutdown);
	VERTEX_SERVER_ADD_ACTION(backup);
	VERTEX_SERVER_ADD_ACTION(collectGarbage);
	VERTEX_SERVER_ADD_ACTION(showStats);
	VERTEX_SERVER_ADD_ACTION(view);
	//VERTEX_SERVER_ADD_ACTION(syncSizes);
	VERTEX_SERVER_ADD_ACTION(sync);
	VERTEX_SERVER_ADD_ACTION(log);
	
	// select ops
	VERTEX_SERVER_ADD_OP(object);
	VERTEX_SERVER_ADD_OP(sizes);
	VERTEX_SERVER_ADD_OP(keys);
	VERTEX_SERVER_ADD_OP(values);
	VERTEX_SERVER_ADD_OP(pairs);
	VERTEX_SERVER_ADD_OP(rm);
	VERTEX_SERVER_ADD_OP(count);
	//VERTEX_SERVER_ADD_OP(html);
}  

int VertexServer_process(VertexServer *self)
{	
	Datum *actionName = (Datum *)CHash_atString_(self->query, "action");

	if (actionName)
	{ 
		VertexAction *action = (VertexAction *)CHash_at_(self->actions, actionName);
		
		if (action)
		{
			return action(self);
		}
	}
	else 
	{
		return VertexServer_api_view(self);
	}

	
	Datum_appendCString_(self->error, "invalid action");

	return -1;
}

void VertexServer_readAnyPostData(VertexServer *self)  
{
	struct evbuffer *evb = self->request->input_buffer;
	Datum_setData_size_(self->post, (const char *)EVBUFFER_DATA(evb), EVBUFFER_LENGTH(evb));
}

void VertexServer_requestHandler(struct evhttp_request *req, void *arg)  
{  
	VertexServer *self = arg;
	const char *uri = evhttp_request_uri(req);
	int result;
	struct evbuffer *buf = evbuffer_new();
	
	self->request = req;
	VertexServer_setupYajl(self); 
	
	VertexServer_readAnyPostData(self);
			
	if (strcmp(uri, "/favicon.ico") == 0)
	{
		evhttp_send_reply(self->request, HTTP_OK, HTTP_OK_MESSAGE, buf);
	}
	else
	/*
	if (strcmp(uri, "/") == 0)
	{
		VertexServer_serveFile_(self, "index.html");
		goto done;
	}
	else
	if (strstr(uri, "/static/") == uri)
	{
		VertexServer_serveFile_(self, uri + strlen("/static/"));
		goto done;
	}
	else
	*/
	{
		VertexServer_parseUri_(self, uri);

		Datum_clear(self->result);
		self->isHtml = 0;
		result = VertexServer_process(self);

		if (result == 0)
		{
			if (Datum_size(self->result))
			{
				Datum_nullTerminate(self->result);
				evbuffer_add_printf(buf, "%s", Datum_data(self->result));
			}
			else
			{
				evbuffer_add_printf(buf, "null");
			}

			evhttp_add_header(self->request->output_headers, "Content-Type", self->isHtml ? "text/html;charset=utf-8" : "application/json;charset=utf-8");
			
			evhttp_send_reply(self->request, HTTP_OK, HTTP_OK_MESSAGE, buf);
		}
		else
		{
			evhttp_add_header(self->request->output_headers, "Content-Type", "application/json;charset=utf-8");
			if (Datum_size(self->error))
			{
				Datum_nullTerminate(self->error); 
				
				yajl_gen_clear(self->yajl);
				yajl_gen_datum(self->yajl, self->error);
				Datum_setYajl_(self->error, self->yajl);
				
				evbuffer_add_printf(buf, "%s", Datum_data(self->error));
				Datum_setSize_(self->error, 0);
			}
			else
			{
				evbuffer_add_printf(buf, "\"unknown error\"");
			}
			
			evhttp_send_reply(self->request, HTTP_SERVERERROR, HTTP_SERVERERROR_MESSAGE, buf);		
		}
	}
	
	evhttp_send_reply_end(self->request);
	evbuffer_free(buf);
	
	#ifdef COMMIT_PERIODICALLY
	VertexServer_commitIfNeeded(self);
	#endif
	
	if(self->requestCount % self->requestsPerSample == 0)
	{
		Log_Printf__("STATS: %i requests, %i bytes to write\n", 
			(int)self->requestCount, (int)PDB_bytesWaitingToCommit(self->pdb));
	}

	self->requestCount ++;
	Pool_freeRefs(self->pool);
	PDB_freeNodes(self->pdb);
}

int VertexServer_api_shutdown(VertexServer *self)
{
	globalVertexServer->shutdown = 1;
	event_loopbreak();
	return 0;
}

void VertexServer_SingalHandler(int s)
{
	if(s == SIGPIPE) 
	{
		Log_Printf("received signal SIGPIPE - ignoring\n");
		return;
	}
	
	Log_Printf_("received signal %i\n", s);
	VertexServer_api_shutdown(globalVertexServer);
}

void VertexServer_registerSignals(VertexServer *self)
{
	signal(SIGABRT, VertexServer_SingalHandler);
	signal(SIGINT,  VertexServer_SingalHandler);
	signal(SIGTERM, VertexServer_SingalHandler);
	signal(SIGPIPE, VertexServer_SingalHandler);
}

void VertexServer_setLogPath_(VertexServer *self, const char *path)
{
	self->logPath = path;
}

void VertexServer_setPidPath_(VertexServer *self, const char *path)
{
	self->pidPath = path;
}

void VertexServer_setIsDaemon_(VertexServer *self, int isDaemon)
{
	self->isDaemon = isDaemon;
}

void VertexServer_setDebug_(VertexServer *self, int aBool)
{
	self->debug = aBool;
}

void VertexServer_writePidFile(VertexServer *self)
{
	FILE *pidFile = fopen(self->pidPath, "w");
	
	if (!pidFile)
	{
		Log_Printf_("Unable to open pid file for writing: %s\n", self->pidPath);
		exit(-1);
	}
	else
	{
		if (fprintf(pidFile, "%i", getpid()) < 0)
		{
			Log_Printf("Error writing to pid file\n");
			exit(-1);
		}
		
		if (fclose(pidFile))
		{
			Log_Printf("Error closing pid file\n");
			exit(-1);
		}
	}
}

void VertexServer_removePidFile(VertexServer *self)
{
	if (self->pidPath)
	{
		if (unlink(self->pidPath))
		{
			Log_Printf("Error removing pid file\n");
		}
	}
}

int VertexServer_openLog(VertexServer *self)
{
	Log_init();

	if (self->logPath)
	{
		Log_setPath_(self->logPath);
		
		if (Log_open())
		{
			Log_Printf_("Unable to open log file for writing: %s\n", self->logPath);
			exit(-1);
		}
		
		Log_Printf_("Logging to %s\n", self->logPath);
	}
	else
	{
		Log_Printf("Logging to stderr\n");
	}
	
	return 0;
}

void VertexServer_runEventLoop(VertexServer *self)
{
	event_init();
	self->httpd = evhttp_start("127.0.0.1", self->port);
	 
	if (!self->httpd)
	{
		Log_Printf("evhttp_start failed - is another copy running on the same port?\n");
		exit(-1);
	}
	
	evhttp_set_timeout(self->httpd, 180);
	evhttp_set_gencb(self->httpd, VertexServer_requestHandler, self);  
	//Log_Printf_("libevent using: %s\n", event_get_action());
	
	while (!self->shutdown)
	{
		event_loop(EVLOOP_ONCE);
	}
}

int VertexServer_run(VertexServer *self)
{  	
	Socket_SetDescriptorLimitToMax();
	VertexServer_setupActions(self);
	VertexServer_openLog(self);
	Log_Printf("VertexServer_run\n");
	
	
	if (self->isDaemon)
	{
		Log_Printf("Running as Daemon\n");
		daemon(0, 0);
		
		if (self->pidPath)
		{
			VertexServer_writePidFile(self);
		}
		else
		{
			Log_Printf("-pid is required when running as daemon\n");
			exit(-1);
		}
	}
	
	//VertexServer_setStaticPath_(self, ".");
	VertexServer_registerSignals(self);
	
	if (PDB_open(self->pdb)) 
	{ 
		Log_Printf("unable to open database file\n");
		return -1; 
	}

	VertexServer_runEventLoop(self);
	
	PDB_commit(self->pdb);
	evhttp_free(self->httpd);
	PDB_close(self->pdb);
	PDB_free(self->pdb);
	
	VertexServer_removePidFile(self);
	
	Log_Printf("shutdown\n\n");
	Log_close();
	return 0;  
}
