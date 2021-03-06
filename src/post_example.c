/*
     This file is part of libmicrohttpd
     (C) 2011 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
 * @file post_example.c
 * @brief example for processing POST requests using libmicrohttpd
 * @author Christian Grothoff
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <microhttpd.h>


/**
 * Invalid method page.
 */
#define METHOD_ERROR "<html><head><title>Illegal request</title></head><body>Go away.</body></html>"

/**
 * Invalid URL page.
 */
#define NOT_FOUND_ERROR "<html><head><title>Not found</title></head><body>Go away.</body></html>"


#define FILE_UPLOAD "<html><form action=\"/do_upload\" enctype=\"multipart/form-data\" method=\"post\"> <p> Please specify a file, or a set of files:<br> <input type=\"file\" name=\"datafile\" size=\"40\"> </p> <div> <input type=\"submit\" value=\"Send\"> </div> </form></html>"

#define UPLOAD_DONE "<html><p>Uploaded file has %zu bytes. <a href=\"/upload\">Next upload</a></p></html>"

#define MAIN_PAGE \
"<html>" \
"<head>" \
"<ul>" \
"<li><a href=\"/upload\">File Upload</a></li>" \
"<li><a href=\"/table\">Table</a></li>" \
"<li><a href=\"/chpass\">Change password</a></li>" \
"</ul>" \
"<p><form action=\"/logout\" method=\"post\">" \
"<input type=\"submit\" value=\"Logout\">" \
"</form></p>" \
"</body>" \
"</html>"

#define TABLE_PAGE_VAR \
"<html>" \
"<head>" \
"<title>My Page</title>" \
"</head>" \
"<body>" \
"<form name=\"myform\" action=\"/table\" method=\"POST\">" \
"<table>" \
"<tr>" \
"<td>1</td>" \
"<td>" \
"<select name=\"dropdown1\">" \
"<option %svalue=\"1\">Auto</option>" \
"<option %svalue=\"2\">100 MB/FD</option>" \
"<option %svalue=\"3\">100 MB/HD</option>" \
"<option %svalue=\"4\">10 MB/FD</option>" \
"<option %svalue=\"5\">10 MB/HD</option>" \
"</select>" \
"</td>" \
"</tr>" \
"<tr>" \
"<td>2</td>" \
"<td>" \
"<select name=\"dropdown2\">" \
"<option %svalue=\"1\">Auto</option>" \
"<option %svalue=\"2\">100 MB/FD</option>" \
"<option %svalue=\"3\">100 MB/HD</option>" \
"<option %svalue=\"4\">10 MB/FD</option>" \
"<option %svalue=\"5\">10 MB/HD</option>" \
"</select>" \
"</td>" \
"</tr>" \
"</table>" \
"<button type=\"submit\">Submit</button>" \
"</form>" \
"</body>" \
"</html>"

#define LOGIN_FORM "<html><body>" \
"<p><form action=\"/\" method=\"post\">" \
"<p>Please login:<br />" \
"Username:<input type=\"text\" name=\"user\" value=\"admin\"><br />" \
"Password:<input name=\"password\" type=\"password\" size=\"12\" maxlength=\"12\"><br />" \
"<input type=\"submit\" value=\"Login\">" \
"</form></p>" \
"</body></html>"

#define LOGOUT_FORM "<html><body>" \
"<p><form action=\"/logout\" method=\"post\">" \
"<input type=\"submit\" value=\"Logout\">" \
"</form></p>" \
"</body></html>"

#define CHPASS_FORM "<html><body>" \
"<p><form action=\"/chpass\" method=\"post\">" \
"<p>New password:<br />" \
"Password:<input name=\"newpassword\" type=\"password\" size=\"12\" maxlength=\"12\"><br />" \
"<input type=\"submit\" value=\"Change\">" \
"</form></p>" \
"</body></html>"

/**
 * Name of our cookie.
 */
#define COOKIE_NAME "session"


/**
 * State we keep for each user/session/browser.
 */
struct Session
{
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

  int logged_in;
};


/**
 * Data kept per request.
 */
struct Request
{

  /**
   * Associated session.
   */
  struct Session *session;

  /**
   * Post processor handling form data (IF this is
   * a POST request).
   */
  struct MHD_PostProcessor *pp;

	void *data;
	const struct PostIterator *pi;
};

struct LoginRequest { 
  char user[64];
  char password[64];
};
struct UploadRequest { 
	size_t file_size;
};
struct TableRequest { 
	struct {
  	char value[2];
	} dropbox[2];
};

struct ChpassRequest { 
  char newpassword[64];
};
/**
 * Linked list of all active sessions.  Yes, O(n) but a
 * hash table would be overkill for a simple example...
 */
static struct Session *sessions;

/**
 * Return the session handle for this connection, or
 * create one if this is a new user.
 */
static struct Session *
get_session(struct MHD_Connection *connection)
{
  struct Session *ret;
  const char *cookie;

  cookie = MHD_lookup_connection_value (connection, MHD_COOKIE_KIND, COOKIE_NAME);
  if (cookie != NULL)
    {
      /* find existing session */
      ret = sessions;
      while (NULL != ret)
	{
	  if (0 == strcmp (cookie, ret->sid))
	    break;
	  ret = ret->next;
	}
      if (NULL != ret)
	{
	  ret->rc++;
	  return ret;
	}
    }
  /* create fresh session */
  ret = calloc (1, sizeof (struct Session));
  if (NULL == ret)
    {
      fprintf (stderr, "calloc error: %s\n", strerror (errno));
      return NULL;
    }
  /* not a super-secure way to generate a random session ID,
     but should do for a simple example... */
  snprintf (ret->sid,
	    sizeof (ret->sid),
	    "%X%X%X%X",
	    (unsigned int) rand (),
	    (unsigned int) rand (),
	    (unsigned int) rand (),
	    (unsigned int) rand ());
  fprintf(stderr, "new session %s\n", ret->sid);
  ret->rc++;
  ret->start = time (NULL);
  ret->next = sessions;
  sessions = ret;
  return ret;
}


/**
 * Type of handler that generates a reply.
 *
 * @param cls content for the page (handler-specific)
 * @param mime mime type to use
 * @param session session information
 * @param connection connection to process
 * @param MHD_YES on success, MHD_NO on failure
 */
typedef int (*PageHandler)(const void *cls,
			   const char *mime,
			   struct Request *request,
			   struct MHD_Connection *connection);


/**
 * Entry we generate for each page served.
 */
struct Page
{
  /**
   * Acceptable URL for this page.
   */
  const char *url;

  /**
   * Mime type to set for the page.
   */
  const char *mime;

  /**
   * Handler to call to generate response.
   */
  PageHandler handler;

  /**
   * Extra argument to handler.
   */
  const void *handler_cls;
};


/**
 * Add header to response to set a session cookie.
 *
 * @param session session to use
 * @param response response to modify
 */
static void
add_session_cookie (struct Session *session,
		    struct MHD_Response *response)
{
  char cstr[256];
  snprintf (cstr,
	    sizeof (cstr),
	    "%s=%s",
	    COOKIE_NAME,
	    session->sid);
  if (MHD_NO ==
      MHD_add_response_header (response,
			       MHD_HTTP_HEADER_SET_COOKIE,
			       cstr))
    {
      fprintf (stderr,
	       "Failed to set session cookie header!\n");
    }
}


/**
 * Handler that returns a simple static HTTP page that
 * is passed in via 'cls'.
 *
 * @param cls a 'const char *' with the HTML webpage to return
 * @param mime mime type to use
 * @param session session handle
 * @param connection connection to use
 */
static int
serve_simple_form (const void *cls,
		   const char *mime,
		   struct Request *request,
		   struct MHD_Connection *connection)
{
  int ret;
  const char *form = cls;
  struct MHD_Response *response;
  struct Session *session = request->session;

  /* return static form */
  response = MHD_create_response_from_buffer (strlen (form), (void *) form, MHD_RESPMEM_PERSISTENT);
  if (NULL == response)
    return MHD_NO;
  add_session_cookie(session, response);
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_ENCODING, mime);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}



/**
 * Handler used to generate a 404 reply.
 *
 * @param cls a 'const char *' with the HTML webpage to return
 * @param mime mime type to use
 * @param session session handle
 * @param connection connection to use
 */
static int
not_found_page (const void *cls,
		const char *mime,
		struct Request *request,
		struct MHD_Connection *connection)
{
  int ret;
  struct MHD_Response *response;

  /* unsupported HTTP method */
  response = MHD_create_response_from_buffer(strlen (NOT_FOUND_ERROR),
					      (void *) NOT_FOUND_ERROR,
					      MHD_RESPMEM_PERSISTENT);
  if (NULL == response)
    return MHD_NO;
  ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_ENCODING, mime);
  MHD_destroy_response(response);
  return ret;
}

struct {
	struct {
		char value[2];
	} dropbox[2];
  char password[64];
} device_data = {
	{ {"1"}, {"1"} }, 
	"admin"
};

#define VAL1(s) ((s.value[0] == '1')?"selected ":"")
#define VAL2(s) ((s.value[0] == '2')?"selected ":"")
#define VAL3(s) ((s.value[0] == '3')?"selected ":"")
#define VAL4(s) ((s.value[0] == '4')?"selected ":"")
#define VAL5(s) ((s.value[0] == '5')?"selected ":"")

static int
table_page(const void *cls, const char *mime,
		 struct Request *request,
		 struct MHD_Connection *connection)
{
  int ret;
  char *reply;
  struct MHD_Response *response;
	const size_t buf_size = 1024*8;
  struct Session *session = request->session;

  /* "selected" */
  reply = malloc(buf_size);
  if (NULL == reply)
    return MHD_NO;

  snprintf(reply, buf_size, TABLE_PAGE_VAR, 
		VAL1(device_data.dropbox[0]),
		VAL2(device_data.dropbox[0]),
		VAL3(device_data.dropbox[0]),
		VAL4(device_data.dropbox[0]),
		VAL5(device_data.dropbox[0]),
		VAL1(device_data.dropbox[1]),
		VAL2(device_data.dropbox[1]),
		VAL3(device_data.dropbox[1]),
		VAL4(device_data.dropbox[1]),
		VAL5(device_data.dropbox[1]));

  /* return static form */
  response = MHD_create_response_from_buffer(strlen(reply), (void *) reply, MHD_RESPMEM_MUST_FREE);
  if (NULL == response)
    return MHD_NO;
  add_session_cookie(session, response);
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_ENCODING, mime);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}
static int
upload_done(const void *cls, const char *mime,
		 struct Request *request,
		 struct MHD_Connection *connection)
{
  int ret;
  char *reply;
  struct MHD_Response *response;
	struct Session *session = request->session;
  struct UploadRequest *ur = (struct UploadRequest *)request->data;

  reply = malloc(strlen(UPLOAD_DONE) + 15 + 1);
  if (NULL == reply)
    return MHD_NO;
  snprintf(reply, strlen(UPLOAD_DONE) + 15 + 1, UPLOAD_DONE, ur->file_size);
  /* return static form */
  response = MHD_create_response_from_buffer(strlen(reply), (void *) reply, MHD_RESPMEM_MUST_FREE);
  if (NULL == response)
    return MHD_NO;
  add_session_cookie(session, response);
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_ENCODING, mime);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}

/**
 * List of all pages served by this HTTP server.
 */
static struct Page pages[] =
  {
    { "/", "text/html", &serve_simple_form, MAIN_PAGE },
    { "/upload", "text/html", &serve_simple_form, FILE_UPLOAD },
    { "/table", "text/html", &table_page, NULL },
    { "/do_upload", "text/html", &upload_done, FILE_UPLOAD },
    { "/login", "text/html", &serve_simple_form, LOGIN_FORM },
    { "/logout", "text/html", &serve_simple_form, LOGOUT_FORM },
    { "/chpass", "text/html", &serve_simple_form, CHPASS_FORM },
    { NULL, NULL, &not_found_page, NULL } /* 404 */
  };



/**
 * Iterator over key-value pairs where the value
 * maybe made available in increments and/or may
 * not be zero-terminated.  Used for processing
 * POST data.
 *
 * @param cls user-specified closure
 * @param kind type of the value
 * @param key 0-terminated key for the value
 * @param filename name of the uploaded file, NULL if not known
 * @param content_type mime-type of the data, NULL if not known
 * @param transfer_encoding encoding of the data, NULL if not known
 * @param data pointer to size bytes of data at the
 *              specified offset
 * @param off offset of data in the overall value
 * @param size number of bytes in data available
 * @return MHD_YES to continue iterating,
 *         MHD_NO to abort the iteration
 */

void to_str(uint64_t off, size_t size, size_t max, const char *data, char *dest)
{
	if (size + off >= max)
		size = max - off - 1;
	memcpy(&dest[off], data, size);
	dest[size+off] = '\0';
}

static int
login_iterator(void *cls,
	       enum MHD_ValueKind kind,
	       const char *key,
	       const char *filename,
	       const char *content_type,
	       const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
	struct LoginRequest *lr = (struct LoginRequest *)request->data;

  if (!strcmp ("user", key)) {
		to_str(off, size, sizeof(lr->user), data, lr->user);		
		return MHD_YES;
	}
  if (!strcmp ("password", key)) {
		to_str(off, size, sizeof(lr->password), data, lr->password);		
		return MHD_YES;
	}
  fprintf (stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}

void login_process(struct Request *request)
{
	struct LoginRequest *lr = (struct LoginRequest *)request->data;
	fprintf(stderr, "login %s %s\n", lr->user, lr->password);
	if (!strcmp("admin", lr->user) && !strcmp(device_data.password, lr->password))
		request->session->logged_in = 1;
}

void logout_process(struct Request *request)
{
	fprintf(stderr, "logout\n");
	request->session->logged_in = 0;
}

static int
upload_iterator(void *cls,
	       enum MHD_ValueKind kind,
	       const char *key,
	       const char *filename,
	       const char *content_type,
	       const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
  struct UploadRequest *ur = (struct UploadRequest *)request->data;

	if (!strcmp(key, "datafile")) {
		ur->file_size += size;
		fprintf(stderr, "datafile request %p offset %d size %d \n", request, (int)off, (int)size);
		return MHD_YES;
	}
  fprintf (stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}
void upload_process(struct Request *request)
{
	struct UploadRequest *ur = (struct UploadRequest *)request->data;
	fprintf(stderr, "file_size %zu\n", ur->file_size);
}

static int
table_iterator(void *cls,
	       enum MHD_ValueKind kind,
	       const char *key,
	       const char *filename,
	       const char *content_type,
	       const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
  struct TableRequest *tr = (struct TableRequest *)request->data;

	if (!strcmp("dropdown1", key)) {
		to_str(off, size, sizeof(tr->dropbox[0].value), data, tr->dropbox[0].value);		
		return MHD_YES;
	}
	if (!strcmp("dropdown2", key)) {
		to_str(off, size, sizeof(tr->dropbox[1].value), data, tr->dropbox[1].value);		
		return MHD_YES;
	}
  fprintf (stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}
void table_process(struct Request *request)
{
	struct TableRequest *tr = (struct TableRequest *)request->data;
	fprintf(stderr, "d1: %s d2: %s\n", tr->dropbox[0].value, tr->dropbox[1].value);
	memcpy(device_data.dropbox, tr->dropbox, sizeof(device_data.dropbox));
}

static int
chpass_iterator(void *cls,
	       enum MHD_ValueKind kind,
	       const char *key,
	       const char *filename,
	       const char *content_type,
	       const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
  struct ChpassRequest *cr = (struct ChpassRequest *)request->data;

	if (!strcmp("newpassword", key)) {
		to_str(off, size, sizeof(cr->newpassword), data, cr->newpassword);		
		return MHD_YES;
	}
  fprintf (stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}
void chpass_process(struct Request *request)
{
	struct ChpassRequest *cr = (struct ChpassRequest *)request->data;
	fprintf(stderr, "new password: %s\n", cr->newpassword);
	strcpy(device_data.password, cr->newpassword);
}


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

const struct PostIterator *find_iter(const char *url)
{
	static const struct PostIterator post_request[] = {
		{ "/",          sizeof(struct LoginRequest),  login_iterator,  login_process,  0 },
		{ "/chpass",    sizeof(struct ChpassRequest), chpass_iterator, chpass_process, 1 },
		{ "/logout",    0,                            NULL,            logout_process, 1 },
		{ "/table",     sizeof(struct TableRequest),  table_iterator,  table_process,  1 },
		{ "/do_upload", sizeof(struct UploadRequest), upload_iterator, upload_process, 1 },
	};
	int i;
	
	for (i=0; i<sizeof(post_request)/sizeof(post_request[0]); i++)
		if (!strcmp(url, post_request[i].url))
			return &post_request[i];
	return NULL;
}

/**
 * Main MHD callback for handling requests.
 *
 * @param cls argument given together with the function
 *        pointer when the handler was registered with MHD
 * @param connection handle identifying the incoming connection
 * @param url the requested url
 * @param method the HTTP method used ("GET", "PUT", etc.)
 * @param version the HTTP version string (i.e. "HTTP/1.1")
 * @param upload_data the data being uploaded (excluding HEADERS,
 *        for a POST that fits into memory and that is encoded
 *        with a supported encoding, the POST data will NOT be
 *        given in upload_data and is instead available as
 *        part of MHD_get_connection_values; very large POST
 *        data *will* be made available incrementally in
 *        upload_data)
 * @param upload_data_size set initially to the size of the
 *        upload_data provided; the method must update this
 *        value to the number of bytes NOT processed;
 * @param ptr pointer that the callback can set to some
 *        address and that will be preserved by MHD for future
 *        calls for this request; since the access handler may
 *        be called many times (i.e., for a PUT/POST operation
 *        with plenty of upload data) this allows the application
 *        to easily associate some request-specific state.
 *        If necessary, this state can be cleaned up in the
 *        global "MHD_RequestCompleted" callback (which
 *        can be set with the MHD_OPTION_NOTIFY_COMPLETED).
 *        Initially, <tt>*con_cls</tt> will be NULL.
 * @return MHS_YES if the connection was handled successfully,
 *         MHS_NO if the socket must be closed due to a serios
 *         error while handling the request
 */
static int
create_response (void *cls,
		 struct MHD_Connection *connection,
		 const char *url,
		 const char *method,
		 const char *version,
		 const char *upload_data,
		 size_t *upload_data_size,
		 void **ptr)
{
  struct MHD_Response *response;
  struct Request *request;
  struct Session *session;
  int ret;
  unsigned int i;

  request = *ptr;
  if (!request) {
		if (!(request = calloc(1, sizeof (struct Request)))) {
			fprintf (stderr, "calloc error: %s\n", strerror (errno));
			return MHD_NO;
		}
		fprintf(stderr, "new request %s %s %p\n", method, url, request);
		*ptr = request;
		return MHD_YES;
	}

  if (!request->session) {
		if (!(request->session = get_session(connection))) {
			fprintf (stderr, "Failed to setup session for `%s'\n", url);
			return MHD_NO; /* internal error */
		}
		if (!strcmp(method, MHD_HTTP_METHOD_POST)) {
			const struct PostIterator *pi = request->pi = find_iter(url);
			if (pi) {
				if (pi->data_size && !(request->data = calloc(1, pi->data_size))) {
					fprintf (stderr, "calloc error: %s\n", strerror (errno));
					return MHD_NO;
				}
				if (pi->iterator && (!pi->need_session || request->session->logged_in)) {
					if (!(request->pp = MHD_create_post_processor(connection, 1024, pi->iterator, request))) {
						fprintf (stderr, "Failed to setup post processor for `%s'\n", url);
						return MHD_NO; /* internal error */
					}
				}
			}
		}
	}
  session = request->session;
  session->start = time(NULL);
  
  if (!strcmp(method, MHD_HTTP_METHOD_POST)) {
		/* evaluate POST data */
		if (request->pp)
			MHD_post_process(request->pp, upload_data, *upload_data_size);

    /* fprintf(stderr, "upload_data_size: %zu\n", *upload_data_size); */
		if (0 != *upload_data_size) {
			*upload_data_size = 0;
    	/* fprintf(stderr, "upload_data_size set to 0\n"); */
			return MHD_YES;
		}
		/* done with POST data, serve response */
		if (request->pp) {
			MHD_destroy_post_processor(request->pp);
			request->pp = NULL;
		}

		fprintf(stderr, "done with POST data %p\n", request);

		if (request->pi && request->pi->process) {
			if (!request->pi->need_session || session->logged_in)
				request->pi->process(request);
		}
		method = MHD_HTTP_METHOD_GET; /* fake 'GET' */
	}

  if ((!strcmp(method, MHD_HTTP_METHOD_GET)) || (!strcmp(method, MHD_HTTP_METHOD_HEAD))) {
      /* find out which page to serve */
			if (!session->logged_in) {
				return serve_simple_form(LOGIN_FORM, "text/html", request, connection);
			} else {
				i=0;
				while ( (pages[i].url != NULL) && (0 != strcmp (pages[i].url, url)) )
					i++;
				ret = pages[i].handler (pages[i].handler_cls, pages[i].mime, request, connection);
				if (ret != MHD_YES)
					fprintf (stderr, "Failed to create page for `%s'\n", url);
				return ret;
			}
    }
  /* unsupported HTTP method */
  response = MHD_create_response_from_buffer (strlen (METHOD_ERROR),
					      (void *) METHOD_ERROR,
					      MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection,
			    MHD_HTTP_METHOD_NOT_ACCEPTABLE,
			    response);
  MHD_destroy_response (response);
  return ret;
}


/**
 * Callback called upon completion of a request.
 * Decrements session reference counter.
 *
 * @param cls not used
 * @param connection connection that completed
 * @param con_cls session handle
 * @param toe status code
 */
static void
request_completed_callback (void *cls,
			    struct MHD_Connection *connection,
			    void **con_cls,
			    enum MHD_RequestTerminationCode toe)
{
  struct Request *request = *con_cls;

  if (NULL == request)
    return;
  if (NULL != request->session)
    request->session->rc--;
  if (NULL != request->pp)
    MHD_destroy_post_processor (request->pp);
	/* fprintf(stderr, "free() request %p\n", request); */
  if (request->data)
		free(request->data);
	free(request);
}


/**
 * Clean up handles of sessions that have been idle for
 * too long.
 */
static void
expire_sessions ()
{
  struct Session *pos;
  struct Session *prev;
  struct Session *next;
  time_t now;

  now = time (NULL);
  prev = NULL;
  pos = sessions;
  while (NULL != pos)
    {
      next = pos->next;
      if (now - pos->start > 60*60) {
	  /* expire sessions after 1h */
	  if (NULL == prev)
	    sessions = pos->next;
	  else
	    prev->next = next;
	  fprintf(stderr, "session %s expired\n", pos->sid);
	  free(pos);
      } else
        prev = pos;
      pos = next;
    }
}


/**
 * Call with the port number as the only argument.
 * Never terminates (other than by signals, such as CTRL-C).
 */
int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;
  struct timeval *tvp;
  fd_set rs;
  fd_set ws;
  fd_set es;
  int max;

  if (argc != 2)
    {
      printf ("%s PORT\n", argv[0]);
      return 1;
    }
  /* initialize PRNG */
  srand ((unsigned int) time (NULL));
  d = MHD_start_daemon (MHD_USE_DEBUG /* |MHD_USE_SSL */,
                        atoi (argv[1]),
                        NULL, NULL,
			&create_response, NULL,
			MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 15,
			MHD_OPTION_NOTIFY_COMPLETED, &request_completed_callback, NULL,
			MHD_OPTION_END);
  if (NULL == d)
    return 1;
  while (1)
    {
      expire_sessions ();
      max = 0;
      FD_ZERO (&rs);
      FD_ZERO (&ws);
      FD_ZERO (&es);
      if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
	break; /* fatal internal error */
#if 0
      if (MHD_get_timeout (d, &mhd_timeout) == MHD_YES)
	{
	  tv.tv_sec = mhd_timeout / 1000;
	  tv.tv_usec = (mhd_timeout - (tv.tv_sec * 1000)) * 1000;
	  tvp = &tv;
	}
      else
#endif
	tvp = NULL;
      select (max + 1, &rs, &ws, &es, tvp);
      MHD_run (d);
    }
  MHD_stop_daemon (d);
  return 0;
}

