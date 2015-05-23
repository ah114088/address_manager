#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#include "request.h"
#include "user.h"
#include "formation.h"
#include "member.h"

#define FORMATION -1

static struct user_struct *user_list = NULL;
static struct user_struct super_user = { NULL, "Superuser", "Superuser", FID_ANY };

struct newuser_form_state {
	int pos;
	const struct formation_struct *f; /* formation iterator */
};

struct user_struct *auth_user(const char *username, const char *password)
{
	struct user_struct *user;
	if (!strcmp(super_user.username, username) && !strcmp(super_user.password, username))
		return &super_user;
	if ((user = find_user(username)) && !strcmp(user->password, password))
		return user;

	return NULL;

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
			"<form action=\"/newuser\" method=\"POST\">" "<table><tr><td>Gliederung:</td><td><select name=\"fid\">");
		nfs->pos++;
		nfs->f = get_formation_list();
		return p - buf;

	case 1:
		if (nfs->f) {
			p += sprintf(p, "<option value=\"%d\">%s</option>", 
				nfs->f->fid, nfs->f->name);
			nfs->f = nfs->f->next;
			return p - buf;
		}
		nfs->pos++;
		/* no break */

	case 2:
		p = stpcpy(p, "</select></td></tr><tr><td>Benutzername:</td><td><input name=\"username\" type=\"text\" size=\"12\" maxlength=\"12\"></td></tr>" \
			"<tr><td>Neues Passwort:</td><td><input name=\"password\" type=\"password\" size=\"12\" maxlength=\"12\">" \
			"</td></tr></table>" \
			"<input type=\"submit\" value=\"Hinzufügen\">" \
			"</form></div>");
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

struct MHD_Response *welcome_form(struct Request *request)
{
  char *reply, *p;

  if (!(reply=malloc(MAXPAGESIZE)))
    return NULL;
		
	p = add_header(reply, "Willkommen");
  p += sprintf(p, "<div id=\"main\"><p>Willkommen %s!</p></div>", request->session->logged_in->username);
	p = add_footer(p);

  return MHD_create_response_from_buffer(p - reply, (void *) reply, MHD_RESPMEM_MUST_FREE);
}
struct MHD_Response *chpass_form(struct Request *request)
{
  char *reply, *p;

  if (!(reply=malloc(MAXPAGESIZE)))
    return NULL;
		
	p = add_header(reply, "Passwort ändern");
  p = stpcpy(p, "<div id=\"main\">" \
		"<p><form action=\"/chpass\" method=\"post\">" \
		"Neues Passwort: <input name=\"newpassword\" type=\"password\" size=\"12\" maxlength=\"12\">" \
		"<input type=\"submit\" value=\"Ändern\">" \
		"</form></p></div>");
	p = add_footer(p);

  return MHD_create_response_from_buffer(p - reply, (void *) reply, MHD_RESPMEM_MUST_FREE);
}

int newuser_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
	struct NewuserRequest *nr = (struct NewuserRequest *)request->data;

	COPY_AND_RETURN(nr, "username", username)
	COPY_AND_RETURN(nr, "password", password)
	COPY_AND_RETURN(nr, "fid", fid)
  fprintf(stderr, "Unsupported form value `%s'\n", key);
  return MHD_NO;
}

int newuser_process(struct Request *request)
{
	struct user_struct *user;
	struct NewuserRequest *nr = (struct NewuserRequest *)request->data;
	int fid;

	if ((user = find_user(nr->username))) {
		fprintf(stderr, "user %s already exists\n", nr->username);
		return MHD_YES;
	}

	if (parse_fid(nr->fid, &fid)<0)
		return MHD_NO;

	if (!(user = (struct user_struct *)malloc(sizeof(struct user_struct)))) {
		fprintf(stderr, "malloc() failure!!\n");
		return MHD_NO;
	}

	strcpy(user->username, nr->username);
	strcpy(user->password, nr->password);
	user->fid = fid;

	user->next = user_list;
	user_list = user;

	fprintf(stderr, "new user %s %s\n", user->username, user->password);
	return MHD_YES;
}

struct user_form_state {
	int pos;
	const struct user_struct *u; /* user iterator */
	int fid; /* selector */
};

/*
    pos:
			0 static
			1 user
			2 static
			3 done
*/
static ssize_t user_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct user_form_state *ufs = cls;
	char *p = buf;

	switch (ufs->pos) {
	case 0:
		p = add_header(p, "Benutzer");
		p = stpcpy(p, "<div id=\"main\">" \
			"<table><tr><th>Benutzername</th><th>Gliederung</th></tr>");
		ufs->pos++;
		ufs->u = user_list;
		return p - buf;

	case 1:
		while (ufs->u) {
			if (in_formation(ufs->u->fid, ufs->fid)) {
				p += sprintf(p, "<tr><td>%s</td><td>%s</td></tr>", ufs->u->username, get_formation(ufs->u->fid)->name);
				ufs->u = ufs->u->next;
				return p - buf;
			}
			ufs->u = ufs->u->next;
		}
		ufs->pos++;
		/* no break */

	case 2:
		p = stpcpy(p, "</table></div>");
		p = add_footer(p);
		ufs->pos++;
		return p - buf;

	default:
		return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
	}	

	return p - buf;
}

struct MHD_Response *user_form(struct Request *request)
{
	struct user_form_state *ufs;
	if (!(ufs = (struct user_form_state *)calloc(1, sizeof(struct user_form_state))))
		return NULL;
	ufs->fid = request->session->logged_in->fid;
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &user_form_reader, ufs, &free);
}
