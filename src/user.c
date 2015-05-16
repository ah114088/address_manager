#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#include "request.h"
#include "user.h"

struct user_struct *user_list = NULL;

struct newuser_form_state {
	int pos;
};

int init_user_list(void)
{
	struct user_struct *user;

	if (!(user = (struct user_struct *)malloc(sizeof(struct user_struct)))) {
		fprintf(stderr, "malloc() failure!!\n");
		return -1;
	}

	user->next = NULL;
	strcpy(user->username, "admin");
	strcpy(user->password, "admin");

	user_list = user;	
	
	return 1;
}

struct user_struct *find_user(const char *username)
{
	struct user_struct *user;

	for (user = user_list; user; user = user->next)
		if (!strcmp(user->username, username))
			return user;
	return NULL;
}

/*
    pos:
			0 static
			1 formation
			2 static
			3 done
*/
static ssize_t newuser_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct newuser_form_state *nfs = cls;
	char *p = buf;

	switch (nfs->pos) {
	case 0:
		p = add_header(p, "Benutzer hinzufügen");
		p = stpcpy(p, "<div id=\"main\">" \
			"<p><form action=\"/newuser\" method=\"post\">" \
			"Benutzername: <input name=\"username\" type=\"text\" size=\"12\" maxlength=\"12\">" \
			"Neues Passwort: <input name=\"password\" type=\"password\" size=\"12\" maxlength=\"12\">" \
			"<input type=\"submit\" value=\"Hinzufügen\">" \
			"</form></p></div>");
		nfs->pos++;
		return p - buf;

	case 1:
		p = add_footer(p);
		nfs->pos++;
		return p - buf;

	default:
		return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
	}	

	return p - buf;
}

struct MHD_Response *newuser_form(struct Request *request)
{
	struct newuser_form_state *nfs;
	if (!(nfs = (struct newuser_form_state *)calloc(1, sizeof(struct newuser_form_state))))
		return NULL;
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &newuser_form_reader, nfs, &free);
}

int newuser_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
	struct NewuserRequest *nr = (struct NewuserRequest *)request->data;

  if (!strcmp("username", key)) {
		to_str(off, size, sizeof(nr->username), data, nr->username);		
		return MHD_YES;
	}
  if (!strcmp("password", key)) {
		to_str(off, size, sizeof(nr->password), data, nr->password);		
		return MHD_YES;
	}
  fprintf(stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}

void newuser_process(struct Request *request)
{
	struct user_struct *user;
	struct NewuserRequest *nr = (struct NewuserRequest *)request->data;
	if ((user = find_user(nr->username))) {
		fprintf(stderr, "user %s already exists\n", nr->username);
		return;
	}

	if (!(user = (struct user_struct *)malloc(sizeof(struct user_struct)))) {
		fprintf(stderr, "malloc() failure!!\n");
		return;
	}

	strcpy(user->username, nr->username);
	strcpy(user->password, nr->password);
	user->next = user_list;
	user_list = user;

	fprintf(stderr, "new user %s %s\n", user->username, user->password);
}
