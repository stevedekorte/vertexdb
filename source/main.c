#include <stdlib.h>
#include "VertexServer.h"

#define HELP_MESSAGE "See vertexdb --help for more information\n"
#define RAISE_ARG_ERROR(a, argc, message) if (a >= argc) { printf("Wrong arguments count or arguments order for: %s\n%s", message, HELP_MESSAGE); return -1; }

int main(int argc, char **argv)  
{  
	int a = 1;
	
	VertexServer *server = VertexServer_new();
	VertexServer_setHardSync_(server, 0);
	while (a < argc)
	{
		if (strcmp(argv[a], "-db") == 0 || strcmp(argv[a], "--database") == 0)
		{
			a ++;
      RAISE_ARG_ERROR(a, argc, argv[a - 1]);
			PDB_setPathCString_(server->pdb, argv[a]);
			a ++;
		} 
		else if (strcmp(argv[a], "-p") == 0 || strcmp(argv[a], "--port") == 0)
		{
			a ++;
			RAISE_ARG_ERROR(a, argc, argv[a - 1]);
			VertexServer_setPort_(server, atoi(argv[a]));
			a ++;
		}
		else if (strcmp(argv[a], "-H") == 0 || strcmp(argv[a], "--host") == 0)
		{
			a ++;
			RAISE_ARG_ERROR(a, argc, argv[a - 1]);
			VertexServer_setHost_(server, argv[a]);
			a ++;
		}
		else if (strcmp(argv[a], "-d") == 0 || strcmp(argv[a], "--daemon") == 0)
		{
			a ++;
			VertexServer_setIsDaemon_(server, 1);
		}
		else if (strcmp(argv[a], "--log") == 0)
		{
			a ++;
			RAISE_ARG_ERROR(a, argc, argv[a - 1]);
			VertexServer_setLogPath_(server, argv[a]);
			a ++;
		}
		else if (strcmp(argv[a], "--pid") == 0)
		{
			a ++;
			RAISE_ARG_ERROR(a, argc, argv[a - 1]);
			VertexServer_setPidPath_(server, argv[a]);
			a ++;
		}
		else if (strcmp(argv[a], "--debug") == 0)
		{
			a ++;
			VertexServer_setDebug_(server, 1);
		}
		else if (strcmp(argv[a], "--hardsync") == 0)
		{
			a ++;
			VertexServer_setHardSync_(server, 1);
		}
		else if (strcmp(argv[a], "--help") == 0 || strcmp(argv[a], "help") == 0 || strcmp(argv[a], "-h") == 0)
		{
      printf("VertexDB usage:\n"
      "    --database <file> -db <file> Database file\n"
      "    --port <num>      -p <num>   TCP port number to listen on (default: 8080)\n"
      "    --host <ip>       -H <ip>    Network interface to listen (default: 127.0.0.1)\n"
      "    --daemon          -d         Run as a daemon\n"
      "    --log <file>                 Log file location\n"
      "    --pid <file>                 Pid file location\n"
      "    --debug                      Be more verbose\n"
      "    --hardsync                   Run with hard syncronization\n"
      "    --help            -h         Show this help\n"
      );
      return 0;
		} else {
      printf("Weird parametr \"%s\"\n%s", argv[a], HELP_MESSAGE);
      a ++;
		}
	}
	
	if (server->isDaemon && !server->pidPath) {
	  printf("--pid is required when running as daemon\n");
    exit(-1);
	}
	
	VertexServer_run(server);
	return 0;  
}
