#ifndef __REQUEST_H
#define __REQUEST_H

#define MAXPAGESIZE 1024
#define MAX_STR 100

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
	int (*process)(struct Request *request);
  int need_session;
};

struct Request {
  struct Session *session; /* Associated session */
  struct MHD_PostProcessor *pp; /* Post processor handling form data (IF this is a POST request) */
	void *data;
	const struct PostIterator *pi;
	int pp_error; /* error while processing POST request */
};

struct Session {
  /**
   * We keep all sessions in a linked list.
   */
  struct Session *next;

  /**
   * Unique ID for this session.
   */
  char sid[33];

  /**
   * Reference counter giving the number of connections
   * currently using this session.
   */
  unsigned int rc;

  /**
   * Time when this session was last active.
   */
  time_t start;

	struct user_struct *logged_in;
};

char *add_header(char *p, const char *title);
char *add_footer(char *p);

int to_str(uint64_t off, size_t size, size_t max, const char *data, char *dest);

#define COPY_AND_RETURN(request, key_str, field)\
  if (!strcmp(key_str, key)) \
		return to_str(off, size, sizeof(request->field), data, request->field);

#endif
