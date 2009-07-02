#include <stdlib.h>
#include "VertexServer.h"

int main(int argc, char **argv)  
{  
	int a = 1;
	
	VertexServer *server = VertexServer_new();
	
	while (a < argc)
	{
		if (strcmp(argv[a], "-db") == 0)
		{
			a ++;
			PDB_setPathCString_(server->pdb, argv[a]);
			a ++;
		} 
		else if (strcmp(argv[a], "-port") == 0)
		{
			a ++;
			VertexServer_setPort_(server, atoi(argv[a]));
			a ++;
		}
		else if (strcmp(argv[a], "-daemon") == 0)
		{
			VertexServer_setIsDaemon_(server, 1);
			a ++;
		}
		else if (strcmp(argv[a], "-log") == 0)
		{
			a ++;
			VertexServer_setLogPath_(server, argv[a]);
			a ++;
		}
		else if (strcmp(argv[a], "-pid") == 0)
		{
			a ++;
			VertexServer_setPidPath_(server, argv[a]);
			a ++;
		}
	}
	
	VertexServer_run(server);
	return 0;  
}
