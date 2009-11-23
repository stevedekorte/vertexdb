#include "VertexServer.h"
#include "Date.h"
#include "Socket.h"
#include "PQuery.h"

int VertexServer_api_amchart(VertexServer *self)
{
	Datum *slot1  = HttpRequest_queryValue_(self->httpRequest, "slot1");
	Datum *slot2  = HttpRequest_queryValue_(self->httpRequest, "slot2");
	Datum *slot3  = HttpRequest_queryValue_(self->httpRequest, "slot3");
	Datum *subpath  = HttpRequest_queryValue_(self->httpRequest, "subpath");
	PNode *node = PDB_allocNode(self->pdb);
	PQuery *q = PNode_query(node);
	VertexServer_setupPQuery_(self, q);
	PNode *tmpNode = PDB_allocNode(self->pdb);
	Datum *d = self->result;
	Datum *title = 0x0;
	//int isFirst = 1;
	
	if (PNode_moveToPathIfExists_(node, HttpRequest_uriPath(self->httpRequest)) != 0) 
	{
		VertexServer_setErrorCString_(self, "path does not exist: ");
		VertexServer_appendError_(self, HttpRequest_uriPath(self->httpRequest));
		return -1;
	}
	
	Datum_appendCString_(d, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	Datum_appendCString_(d, "<chart approx_count=\"");
	
	// series -----------------
	if(Datum_size(subpath)) PNode_moveToPathIfExists_(node, subpath);
	Datum_appendLong_(d, PNode_size(node));
	Datum_appendCString_(d, "\">\n");
	PNode_startQuery(node);
	PNode_amSeries(node, d);
	PNode_moveToPathIfExists_(node, HttpRequest_uriPath(self->httpRequest));
	// series -----------------
	
	Datum_appendCString_(d, "<graphs>\n");

	PNode_first(node);

	
	if(Datum_size(slot1))
	{
		if(Datum_size(slot1)) PNode_amGraphKey_(node, title, slot1, d); 
		if(Datum_size(slot2)) PNode_amGraphKey_(node, title, slot2, d); 
		if(Datum_size(slot3)) PNode_amGraphKey_(node, title, slot3, d); 
	}
	else 
	{
		// this is just to get get the first node so we can see what keys should be in the graph
		if(PNode_value(node))
		{
			title = Datum_poolNew();
			Datum_copy_(title, PNode_key(node));
			PNode_setPid_(tmpNode, PNode_value(node));

			if(Datum_size(subpath))
			{
				PNode_moveToPathIfExists_(tmpNode, subpath);
			}
			
			// enumerate the keys and output graph for each
			PNode_first(tmpNode);
			
			Datum *k;
			while(k = PNode_key(tmpNode))
			{
				PNode_startQuery(node);
				PNode_amGraphKey_(node, title, k, d); 
				PNode_next(tmpNode);
			}
			
		}
		
	}
	
	Datum_appendCString_(d, "</graphs>\n");

	Datum_appendCString_(d, "</chart>\n");

	HttpResponse_setContentType_(self->httpResponse, "text/xml");
	//HttpResponse_setContentType_(self->httpResponse, "text/xml; charset=utf-8");
	return 0;
}


int VertexServer_api_ampie(VertexServer *self)
{
	Datum *slot1  = HttpRequest_queryValue_(self->httpRequest, "slot1");
	//Datum *slot2  = HttpRequest_queryValue_(self->httpRequest, "slot2");
	//Datum *slot3  = HttpRequest_queryValue_(self->httpRequest, "slot3");
	//Datum *subpath  = HttpRequest_queryValue_(self->httpRequest, "subpath");
	PNode *node = PDB_allocNode(self->pdb);
	PQuery *q = PNode_query(node);
	VertexServer_setupPQuery_(self, q);
	PNode *tmpNode = PDB_allocNode(self->pdb);
	Datum *d = self->result;
	Datum *k;
	
	if (PNode_moveToPathIfExists_(node, HttpRequest_uriPath(self->httpRequest)) != 0) 
	{
		VertexServer_setErrorCString_(self, "path does not exist: ");
		VertexServer_appendError_(self, HttpRequest_uriPath(self->httpRequest));
		return -1;
	}
		
	//PNode_startQuery(node);
	
	PNode_first(node);
	while(k = PNode_key(node))
	{
		PNode_setPid_(tmpNode, PNode_value(node));
		Datum *v = PNode_at_(tmpNode, slot1);
		
		if(v)
		{
			Datum_append_(d, k);
			Datum_appendCString_(d, ";");
			Datum_append_(d, v);
			Datum_appendCString_(d, "\n");
		}
		
		PNode_next(node);
	}

	HttpResponse_setContentType_(self->httpResponse, "text/plain");
	//HttpResponse_setContentType_(self->httpResponse, "text/xml; charset=utf-8");
	return 0;
}
