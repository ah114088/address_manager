#ifndef __REQUEST_H
#define __REQUEST_H

struct Request;
struct PostIterator {
	const char *url;
	size_t data_size;
	int (*iterator)(void *cls,
	       enum MHD_ValueKind kind,
	       const char *key,
	       const char *filename,
	       const char *content_type,
	       const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size);
	void (*process)(struct Request *request);
  int need_session;
};

struct Request {
  struct Session *session; /* Associated session */
  struct MHD_PostProcessor *pp; /* Post processor handling form data (IF this is a POST request) */
	void *data;
	const struct PostIterator *pi;
};

char *add_header(char *p, const char *title);
char *add_footer(char *p);

#endif
