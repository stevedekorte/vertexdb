

/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void VertexServer_serveFile_(VertexServer *self, const char *path)
{		
	Datum_copy_(self->filePath, self->staticPath);
	Datum_appendCString_(self->filePath, path);
	
	printf("file request: %s\n", Datum_data(self->filePath));
	
	{
		int fd = open(Datum_data(self->filePath), O_RDONLY);
		evbuffer_read(self->buf, fd, 100000);
		close(fd);
	}
	
	VertexServer_sendOk(self);
}
*/
