#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <microhttpd.h>

#include "file.h"

struct MHD_Response *file_based_response(const char *url, const char **mime)
{
	int i, fd;
	struct stat sb;
	char fname[255];
	const char *dot;

	static const struct {
	  const char *file_ext;
	  const char *mime;
	} file_type[] = {
		{ "png",  "image/png" },
		{ "jpeg", "image/jpeg" },
		{ "jpg",  "image/jpeg" },
		{ "gif",  "image/gif" },
		{ "html", "text/html" },
		{ "css",  "text/css" },
	};
	
	if (!(dot = strrchr(url, '.')))
		return NULL;

	if (url[0] != '/')
		return NULL;
	url++; // skip '/'
	
	for (i=0; i<sizeof(file_type)/sizeof(file_type[0]); i++) {
		if (!strcmp(&dot[1], file_type[i].file_ext)) {
			struct MHD_Response *response;
			snprintf(fname, sizeof(fname), "./resources/%s", url);
			if (stat(fname, &sb) || ((fd=open(fname, O_RDONLY))==-1))
				return NULL;
			*mime = file_type[i].mime;
			response = MHD_create_response_from_fd_at_offset(sb.st_size, fd, 0);
			return response;
		}
	}
	
	return NULL;
}
