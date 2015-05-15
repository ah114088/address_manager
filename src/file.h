#ifndef __FILE_H
#define __FILE_H

struct MHD_Response *file_based_response(const char *url, const char **mime);

#endif
