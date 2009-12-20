/*
	Notes:
	See http://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html 
	for valid response headers

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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "Date.h"
#include "Socket.h"
#include "PQuery.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#define USER_ID_LENGTH 12
//#define COMMIT_PERIODICALLY 1

static VertexServer *globalVertexServer = 0x0;

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
	srand((clock() % INT_MAX));
	
	self->pdb   = PDB_new();
	VertexServer_setupYajl(self);
	self->httpServer = HttpServer_new();
	HttpServer_setPort_(self->httpServer, 8080);
	HttpServer_setHost_(self->httpServer, "127.0.0.1");
	HttpServer_setDelegate_(self->httpServer, self);
	HttpServer_setRequestCallback_(self->httpServer, VertexServer_requestHandler);

	self->httpRequest = HttpServer_request(self->httpServer);
	self->httpResponse = HttpServer_response(self->httpServer);
	self->result = HttpResponse_content(self->httpResponse);

	self->actions = CHash_new();
	CHash_setEqualFunc_(self->actions, (CHashEqualFunc *)Datum_equals_);
	CHash_setHash1Func_(self->actions, (CHashHashFunc *)Datum_hash1);
	CHash_setHash2Func_(self->actions, (CHashHashFunc *)Datum_hash2);
	
	self->ops = CHash_new();
	CHash_setEqualFunc_(self->ops, (CHashEqualFunc *)Datum_equals_);
	CHash_setHash1Func_(self->ops, (CHashHashFunc *)Datum_hash1);
	CHash_setHash2Func_(self->ops, (CHashHashFunc *)Datum_hash2);
			
	return self;
}

void VertexServer_free(VertexServer *self)
{
	PDB_free(self->pdb);
	HttpServer_free(self->httpServer);
	Datum_freePool();
	PNode_freePool();
	
	CHash_free(self->actions);
	CHash_free(self->ops);
	
	if (self->yajl)
	{
		yajl_gen_free(self->yajl);
	}
	
	free(self);
}


void VertexServer_setPort_(VertexServer *self, int port)
{
	HttpServer_setPort_(self->httpServer, port);
}

void VertexServer_setErrorCString_(VertexServer *self, const char *s)
{
	HttpResponse_setContentCString_(self->httpResponse, s);
	HttpResponse_setStatusCode_(self->httpResponse, 500);
}

void VertexServer_appendError_(VertexServer *self, Datum *d)
{
	HttpResponse_appendContent_(self->httpResponse, d);
}


/*
void VertexServer_showUri(VertexServer *self)
{
	printf("request: '%s'\n", Datum_data(HttpRequest_uriPath(self->httpRequest)));
}
*/


// --- API -------------------------------------------------------------

int VertexServer_api_setCursorPathOnNode_(VertexServer *self, PNode *node)
{	
	int r = PNode_moveToPathIfExists_(node, HttpRequest_uriPath(self->httpRequest));
	
	if (r)
	{
		VertexServer_setErrorCString_(self, "path does not exist: ");
		VertexServer_appendError_(self, HttpRequest_uriPath(self->httpRequest));
	}
	
	return r;
}

// ------------------------------------

int VertexServer_api_size(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;	
	Datum_appendLong_(self->result, (long)PNode_size(node));
	return 0;
}

int VertexServer_api_chmod(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	//Datum *value = HttpRequest_queryValue_(self->httpRequest, "mode");
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	//PNode_atPut_(node, key, value);
	return 0;
}

int VertexServer_api_chown(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	//Datum *owner = HttpRequest_queryValue_(self->httpRequest, "owner");
	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	
	/*
	if(!PNode_hasOwner_(node, VertexServer_user(self))
	{
		 "only the owner can change this node's owner"
		return -1;
	}
	*/
	
	//PNode_setOwner_(node, owner);

	return 0;
}

void VertexServer_setupPQuery_(VertexServer *self, PQuery *q)
{
	PQuery_setId_(q, 
		HttpRequest_queryValue_(self->httpRequest, "id"));
	PQuery_setAfter_(q, 
		HttpRequest_queryValue_(self->httpRequest, "after"));
	PQuery_setBefore_(q, 
		HttpRequest_queryValue_(self->httpRequest, "before"));
	PQuery_setSelectCountMax_(q, 
		Datum_asInt(HttpRequest_queryValue_(self->httpRequest, "count")));
	PQuery_setWhereKey_(q, 
		HttpRequest_queryValue_(self->httpRequest, "whereKey"));
	PQuery_setWhereValue_(q, 
		HttpRequest_queryValue_(self->httpRequest, "whereValue"));
	PQuery_setAttribute_(q, 
		HttpRequest_queryValue_(self->httpRequest, "attribute"));
	//PQuery_setMode_(q, HttpRequest_queryValue_(self->httpRequest, "mode"));
}

int VertexServer_api_select(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	PQuery *q = PNode_query(node);
	VertexServer_setupPQuery_(self, q);
	Datum *opName;
	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	
	opName = HttpRequest_queryValue_(self->httpRequest, "op");

	if (opName)
	{ 
		PNodeOp *op = (PNodeOp *)CHash_at_(self->ops, opName);
		
		if (op)
		{
			return op(node, self->result);
		}
	}

	VertexServer_setErrorCString_(self, "invalid node op");
	
	return -1;
}

int VertexServer_api_show(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;
	Datum_appendLong_(self->result, (long)PNode_size(node));
	return 0;
}

int VertexServer_api_rm(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	Datum *key = HttpRequest_queryValue_(self->httpRequest, "key");	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 0; // ignore if it doesn't exist
	PNode_removeAt_(node, key);
	return 0;
}

int VertexServer_api_read(VertexServer *self)
{
	PNode *node = PDB_allocNode(self->pdb);
	Datum *key = HttpRequest_queryValue_(self->httpRequest, "key");	
	Datum *mode = HttpRequest_queryValue_(self->httpRequest, "mode");	
	Datum *value;
	
	if (VertexServer_api_setCursorPathOnNode_(self, node)) return 2;

	if(Datum_equalsCString_(mode, "meta"))
	{
		value = PNode_metaAt_(node, key);
	}
	else 
	{
		value = PNode_at_(node, key);
	}

	
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
	Datum *mode  = HttpRequest_queryValue_(self->httpRequest, "mode");
	Datum *key   = HttpRequest_queryValue_(self->httpRequest, "key");
	Datum *value = HttpRequest_queryValue_(self->httpRequest, "value");
	Datum *post  = HttpRequest_postData(self->httpRequest);
	
	if(Datum_size(post) != 0)
	{
		value = post;
	}


	if (PNode_moveToPathIfExists_(node, HttpRequest_uriPath(self->httpRequest)) != 0) 
	{
		VertexServer_setErrorCString_(self, "write path does not exist: ");
		VertexServer_appendError_(self, HttpRequest_uriPath(self->httpRequest));
		return -1;
	}	
	
	if(Datum_equalsCString_(mode, "append"))
	{
		PNode_atCat_(node, key, value);
	}
	else if(Datum_equalsCString_(mode, "meta"))
	{
		PNode_metaAt_put_(node, key, value);
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
	
	Datum *key      = HttpRequest_queryValue_(self->httpRequest, "key");
	Datum *fromPath = HttpRequest_uriPath(self->httpRequest);
	Datum *toPath   = HttpRequest_queryValue_(self->httpRequest, "toPath");

	if (PNode_moveToPathIfExists_(toNode, toPath) != 0) 
	{
		VertexServer_setErrorCString_(self, "to path does not exist: ");
		VertexServer_appendError_(self, toPath);
		return -1;
	}
		
	if (PNode_moveToPathIfExists_(fromNode, fromPath) != 0) 
	{
		VertexServer_setErrorCString_(self, "from path does not exist: ");
		VertexServer_appendError_(self, fromPath);
		return -1;
	}	

	PNode_atPut_(toNode, key, PNode_pid(fromNode));

	return 0;
}

int VertexServer_api_transaction(VertexServer *self)
{
	Datum *uri = Datum_new();
	Datum *post = Datum_new();
	int error = 0;
	int r, result;
	
	PDB_commit(self->pdb);
	
	Datum_copy_(post, HttpRequest_postData(self->httpRequest));
	Datum_clear(HttpRequest_postData(self->httpRequest));
	
	do
	{
		Datum_copy_(uri, post);
		r = Datum_sepOnChars_with_(uri, "\n", post);

		if (Datum_size(uri) == 0) 
		{
			VertexServer_setErrorCString_(self, "empty line in transaction");
			error = 1;
			break;
		}
		
		HttpRequest_parseUri_(self->httpRequest, Datum_data(uri));
		error = VertexServer_process(self);
		PNode_poolFreeRefs();
		Datum_poolFreeRefs();
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
	Datum_free(post);
	return result;
}

int VertexServer_api_mkdir(VertexServer *self)
{
	PNode_moveToPath_(PDB_allocNode(self->pdb), HttpRequest_uriPath(self->httpRequest));
	return 0;
}

int VertexServer_api_queuePopTo(VertexServer *self)
{
	PNode *fromNode = PDB_allocNode(self->pdb);
	PNode *toNode   = PDB_allocNode(self->pdb);
	Datum *toPath   = HttpRequest_queryValue_(self->httpRequest, "toPath");
	
	long ttl = Datum_asLong(HttpRequest_queryValue_(self->httpRequest, "ttl"));
	
	if (PNode_moveToPathIfExists_(fromNode, HttpRequest_uriPath(self->httpRequest)) != 0) 
	{
		VertexServer_setErrorCString_(self, "from path does not exist: ");
		VertexServer_appendError_(self, HttpRequest_uriPath(self->httpRequest));
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
				
				Datum *timeKey   = Datum_poolNewWithCString_("_qtime");
				Datum *timeValue = Datum_poolNew();
				
				Datum_fromLong_(timeValue, now);
				PNode_atPut_(toNode, timeKey, timeValue);
				
				Datum_setCString_(timeKey, "_qexpire");
				Datum_fromLong_(timeValue, now + (ttl == 0 ? 3600 : ttl));
				PNode_atPut_(toNode, timeKey, timeValue);
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
	Datum *toPath = HttpRequest_queryValue_(self->httpRequest, "toPath");
	unsigned int itemsExpired = 0;
	
	if (PNode_moveToPathIfExists_(fromNode, HttpRequest_uriPath(self->httpRequest)) != 0) 
	{
		VertexServer_setErrorCString_(self, "from path does not exist: ");
		VertexServer_appendError_(self, HttpRequest_uriPath(self->httpRequest));
		return -1;
	}
	
	//PNode_moveToPath_(toNode, toPath);
	if (PNode_moveToPathIfExists_(toNode, toPath) != 0) 
	{
		VertexServer_setErrorCString_(self, "to path does not exist: ");
		VertexServer_appendError_(self, toPath);
		return -1;
	}
	
	
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
				Log_Printf("WARNING: attempt to expire a node with no _qexpire value\n");
 
				if(PNode_at_(itemNode, qTimeKey) == 0x0)
				{
					Log_Printf("WARNING: node also missing _qtime value\n");
				}
				
				break;
			}
			
			if(qExpireValue == 0x0 || Datum_asLong(qExpireValue) < now)
			{
				PNode_removeAtCursor(fromNode); // the remove will go to the next item
				PNode_key(fromNode);
				PNode_removeAt_(itemNode, qTimeKey);
				PNode_removeAt_(itemNode, qExpireKey);
				PNode_atPut_(toNode, k, pid);
				PNode_jumpToCurrentKey(fromNode);
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


int VertexServer_backup(VertexServer *self)
{
	time_t now = time(NULL);
	int result;
	
	Log_Printf_("backup at %i bytes written... ", (int)PDB_bytesWaitingToCommit(self->pdb));
	
	PDB_commit(self->pdb);
	
	result = PDB_backup(self->pdb);

	Log_Printf__("backup %s and took %i seconds\n", 
		result ? "failed" : "successful", (int)difftime(time(NULL), now));
				
	return result;
}

/*
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
*/

/*
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
*/

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
	printf("%s\n", Datum_data(HttpRequest_postData(self->httpRequest)));
	printf("\n---------------------------------\n");
	return 0;
}

int VertexServer_api_view(VertexServer *self)
{
	Datum *before = HttpRequest_queryValue_(self->httpRequest, "before");
	Datum *after = HttpRequest_queryValue_(self->httpRequest, "after");
	Datum *mode = HttpRequest_queryValue_(self->httpRequest, "mode");
	PNode *node = PDB_allocNode(self->pdb);
	PNode *tmpNode = PDB_allocNode(self->pdb);
	Datum *d = self->result;
	int maxCount = 200;
	
	
	if (PNode_moveToPathIfExists_(node, HttpRequest_uriPath(self->httpRequest)) != 0) 
	{
		VertexServer_setErrorCString_(self, "path does not exist: ");
		VertexServer_appendError_(self, HttpRequest_uriPath(self->httpRequest));
		return -1;
	}
		Datum_appendCString_(d, "<!DOCTYPE html>\n");
		Datum_appendCString_(d, "<html>\n");
		Datum_appendCString_(d, "<head>\n");
		Datum_appendCString_(d, "<title>");
		Datum_append_(d, HttpRequest_uriPath(self->httpRequest));
		Datum_appendCString_(d, "</title>\n");
		Datum_appendCString_(d, "<meta charset=\"utf-8\">\n");
		Datum_appendCString_(d, "<style>");
		Datum_appendCString_(d, "body, td { font-family: Helvetica; font-size: 12px; margin-top:2em; margin-left:2em; }");
		Datum_appendCString_(d, ".path { font-weight: normal; }");
		Datum_appendCString_(d, ".note { color:#aaaaaa; }");
		Datum_appendCString_(d, ".key { color:#000000;  }");
		Datum_appendCString_(d, ".value { color:#888888; white-space:pre; }");
		Datum_appendCString_(d, "a { color: #0000aa; text-decoration: none;  }");

		Datum_appendCString_(d, "</style>\n");
		Datum_appendCString_(d, "</head>\n");
		Datum_appendCString_(d, "<body>\n");
	
	/*
	if(Datum_size(HttpRequest_uriPath(self->httpRequest)) == 0)
	{
	Datum_appendCString_(d, "/");
	}
	else
	{
	*/
		Datum_appendCString_(d, "<font class=path>");
		Datum_appendCString_(d, "/");
		Datum_append_(d, HttpRequest_uriPath(self->httpRequest));
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
		Datum_append_(d, Datum_asUriEncoded(HttpRequest_uriPath(self->httpRequest)));
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
			
			if (Datum_beginsWithCString_(k , "_"))
			{
				Datum_appendCString_(d, "<tr>");
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
				Datum_appendCString_(d, "</tr>");
			}
			else
			{
				if(Datum_equalsCString_(mode, "table"))
				{
					PNode_setPid_(tmpNode, PNode_value(node));
					PNode_asHtmlRow(tmpNode, d);
				}
				else 
				{
					Datum_appendCString_(d, "<tr>");
					Datum_appendCString_(d, "<td align=right>");
					Datum_appendCString_(d, "<font class=key>");
					Datum_append_(d, k);
					Datum_appendCString_(d, "</font><br>\n");
					Datum_appendCString_(d, "</td>");
					
					Datum_appendCString_(d, "<td style=\"line-height:1.5em\">");
					Datum_appendCString_(d, "&nbsp;&nbsp;<a href=");
					if(Datum_size(HttpRequest_uriPath(self->httpRequest)) != 0) Datum_appendCString_(d, "/");
					Datum_append_(d, Datum_asUriEncoded(HttpRequest_uriPath(self->httpRequest)));
					Datum_appendCString_(d, "/");
					Datum_append_(d, Datum_asUriEncoded(k));
					Datum_appendCString_(d, "> ↠ ");
					Datum_appendLong_(d, PNode_nodeSizeAtCursor(node));
					Datum_appendCString_(d, "</a> ");
					Datum_appendCString_(d, "<font class=value>");
					Datum_appendCString_(d, "</td>");
					Datum_appendCString_(d, "</tr>");
				}
			}
			
			PNode_next(node);
			count ++;
		}
	}
	Datum_appendCString_(d, "</table>");
	Datum_appendCString_(d, "</ul>\n");
	
	if(PNode_key(node))
	{
		Datum_appendCString_(d, "<a href=/");
		Datum_append_(d, HttpRequest_uriPath(self->httpRequest));
		Datum_appendCString_(d, "?after=");
		Datum_append_(d, PNode_key(node));
		Datum_appendCString_(d, ">next</a><br>");
	}
	
	Datum_appendCString_(d, "</body>\n");
	Datum_appendCString_(d, "</html>\n");

	HttpResponse_setContentType_(self->httpResponse, "text/html; charset=utf-8");
	return 0;
}

// ---------------------------------------------------------------------

#define VERTEX_SERVER_ADD_ACTION(name) CHash_at_put_(self->actions, Datum_newWithCString_(#name ""), (void *)VertexServer_api_##name);
#define VERTEX_SERVER_ADD_OP(name) CHash_at_put_(self->ops, Datum_newWithCString_(#name ""), (void *)PNode_op_##name);

int VertexServer_api_amchart(VertexServer *self);
int VertexServer_api_ampie(VertexServer *self);

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
	//VERTEX_SERVER_ADD_ACTION(login);
	//VERTEX_SERVER_ADD_ACTION(newUser);
	
	VERTEX_SERVER_ADD_ACTION(chmod);
	VERTEX_SERVER_ADD_ACTION(chown);

	// management
	VERTEX_SERVER_ADD_ACTION(shutdown);
	VERTEX_SERVER_ADD_ACTION(backup);
	VERTEX_SERVER_ADD_ACTION(collectGarbage);
	VERTEX_SERVER_ADD_ACTION(showStats);
	VERTEX_SERVER_ADD_ACTION(view);
	VERTEX_SERVER_ADD_ACTION(amchart);
	VERTEX_SERVER_ADD_ACTION(ampie);
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
	Datum *actionName = (Datum *)HttpRequest_queryValue_(self->httpRequest, "action");

	//if(self->debug) { Log_Printf_("REQUEST ERROR: %s\n", HttpRequest_uri(self->httpRequest)); }

	if (Datum_size(actionName))
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
	
	VertexServer_setErrorCString_(self, "invalid action");

	return -1;
}

void VertexServer_requestHandler(void *arg)  
{  
	VertexServer *self = (VertexServer *)arg;
	VertexServer_setupYajl(self); 
	
	HttpResponse_setContentType_(self->httpResponse, "application/json;charset=utf-8");

	int result = VertexServer_process(self);
	Datum *content = HttpResponse_content(self->httpResponse);
	
	if (result == 0)
	{
		if (!Datum_size(content)) 
		{
			Datum_setCString_(content, "null");
		}
		Datum_nullTerminate(content); 
	}
	else
	{
		if (!Datum_size(content)) 
		{
			Datum_setCString_(content, "\"unknown error\"");
		}
		Datum_nullTerminate(content); 
		
		yajl_gen_clear(self->yajl);
		yajl_gen_datum(self->yajl, content);
		Datum_setYajl_(content, self->yajl);
				
		if(self->debug) { Log_Printf_("REQUEST ERROR: %s\n", Datum_data(content)); }
	}
}

int VertexServer_api_shutdown(VertexServer *self)
{
	HttpServer_shutdown(self->httpServer);
	return 0;
}

void VertexServer_SignalHandler(int s)
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
	signal(SIGABRT, VertexServer_SignalHandler);
	signal(SIGINT,  VertexServer_SignalHandler);
	signal(SIGTERM, VertexServer_SignalHandler);
	signal(SIGPIPE, VertexServer_SignalHandler);
}

void VertexServer_enableCoreDumps(VertexServer *self)
{
	struct rlimit limit;
	limit.rlim_cur = limit.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &limit);
	//printf("RLIMIT_CORE limit.rlim_max = %i\n", (int)limit.rlim_max);
}

void VertexServer_setLogPath_(VertexServer *self, const char *path)
{
	Log_setPath_(path);
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

void VertexServer_setHardSync_(VertexServer *self, int aBool)
{
	PDB_setHardSync_(self->pdb, aBool);
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
	Log_open();
	return 0;
}

int VertexServer_run(VertexServer *self)
{  	
	Socket_SetDescriptorLimitToMax();
	VertexServer_enableCoreDumps(self);
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
	
	VertexServer_registerSignals(self);
		
	if (PDB_open(self->pdb)) 
	{ 
		Log_Printf("unable to open database file\n");
		return -1; 
	}

	HttpServer_run(self->httpServer);
	
	PDB_commit(self->pdb);
	PDB_close(self->pdb);
	
	VertexServer_removePidFile(self);
	
	Log_Printf("shutdown\n\n");
	Log_close();
	return 0;  
}

/*
void VertexServer_vendCookie(VertexServer *self)
{
	Datum *cookie = Datum_poolNew();
	Datum_setCString_(cookie, "userId=");
	Datum_append_(cookie, self->userId);
	Datum_appendCString_(cookie, "; expires=Fri, 31-Dec-2200 12:00:00 GMT; path=/;");
	HttpResponse_setCookie_(self->httpResponse, cookie);
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
	Datum *userPath = Datum_poolNew();
	Datum *userId = Datum_poolNew();
	
	Datum *email = HttpRequest_queryValue_(self->httpRequest, "email");
	Datum *inputPassword = HttpRequest_queryValue_(self->httpRequest, "password");
	Datum *v;
	PNode *node = PDB_allocNode(self->pdb);
	
	PNode_moveToPathCString_(node, "indexes/user/email");
	v = PNode_at_(node, email);
	
	if (!v)
	{
		VertexServer_setErrorCString_(self, "unknown user email address");
		VertexServer_appendError_(self, email);
		return 2;
	}
	
	Datum_copy_(self->userId, v);
	Datum_setCString_(userPath, "users/");
	Datum_append_(userPath, userId);
	PNode_moveToPathIfExists_(node, userPath);
	{
		Datum *password = PNode_atCString_(node, "_password");
		
		if (!password || !Datum_equals_(password, inputPassword))
		{
			VertexServer_setErrorCString_(self, "wrong password");
			return 2; // why 2?
		}
	}
	
	VertexServer_vendCookie(self);
	return 0;
}
*/

