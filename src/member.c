#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#include "request.h"
#include "member.h"
#include "user.h"
#include "formation.h"

struct member_struct {
	struct member_struct *next;
	char *first;
	char *second;
	char *street;
	char *house;
	char *zip;
	char *city;
	char *email;
	int formation;
};

static struct member_struct *member_list = NULL;
static int nmember;

static int cmp_member(const struct member_struct *a, const struct member_struct *b)
{
	int ret;
	if ((ret=strcmp(a->second, b->second))) 
		return ret;
	return strcmp(a->first, b->first);
}

static void add_member(struct member_struct *new)
{
	struct member_struct **mp;

	nmember++;
	for (mp = &member_list; *mp; mp = &(*mp)->next) {
		if (cmp_member(*mp, new) > 0) {
			new->next = *mp;
			*mp = new;
			return;
		}
	}
	new->next = NULL;
	*mp = new;
	return;
}

// UTF-8
int read_member_list(const char *path)
{
	FILE *file;
	char buffer[8][MAX_STR];
	int i = 0;
	struct member_struct *m; 

	if (!(file=fopen(path, "r"))) 
		return -1;

	while (fscanf(file, "%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]\n", 
			buffer[0], buffer[1], buffer[2], buffer[3],
			buffer[4], buffer[5], buffer[6], buffer[7]) == 8) {

		if (!(m = (struct member_struct *)malloc(sizeof(struct member_struct)))) { 
			fprintf(stderr, "malloc() failure!!\n");
			fclose(file);
			return -1;
		}
		m->first     = strdup(buffer[0]);
		m->second    = strdup(buffer[1]);
		m->street    = strdup(buffer[2]);
		m->house     = strdup(buffer[3]);
		m->zip       = strdup(buffer[4]);
		m->city      = strdup(buffer[5]);
		m->email     = strdup(buffer[6]);
		if (sscanf(buffer[7], "%d", &m->formation)!=1) {
			fprintf(stderr, "bad formation number!!\n");
			fclose(file);
			return -1;
		}
		add_member(m);

		/* fprintf(stderr, "read: %s %s\n", m->first, m->second); */
		i++;
	}
		
	fclose(file);

	return i;
}

int member_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
	struct MemberRequest *mr = (struct MemberRequest *)request->data;
	COPY_AND_RETURN(mr, "fid", fid)
  fprintf(stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}
int member_process(struct Request *request)
{
	int fid;
	struct MemberRequest *mr = request->data;
	if (parse_fid(mr->fid, &fid) < 0 || !in_formation(fid, request->session->logged_in->fid))
  	return MHD_NO;
  return MHD_YES;
}

int newmember_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
	struct NewMemberRequest *nr = (struct NewMemberRequest *)request->data;

	COPY_AND_RETURN(nr, "first", first)
	COPY_AND_RETURN(nr, "second", second)
	COPY_AND_RETURN(nr, "street", street)
	COPY_AND_RETURN(nr, "house", house)
	COPY_AND_RETURN(nr, "zip", zip)
	COPY_AND_RETURN(nr, "city", city)
	COPY_AND_RETURN(nr, "email", email)
	COPY_AND_RETURN(nr, "fid", fid)
  fprintf(stderr, "Unsupported form value `%s'\n", key);
  return MHD_YES;
}

int newmember_process(struct Request *request)
{
	struct member_struct *member;
	struct NewMemberRequest *nr = request->data;
	int fid;

	/* TODO: check duplicate member */

	if (parse_fid(nr->fid, &fid)<0)
		return MHD_NO;

	if (!(member = (struct member_struct *)malloc(sizeof(struct member_struct)))) {
		fprintf(stderr, "malloc() failure!!\n");
		return MHD_NO;
	}

	/* TODO: check data consistency */
	/* TODO: check malloc failure */
	member->first = strdup(nr->first);
	member->second = strdup(nr->second);
	member->street = strdup(nr->street);
	member->house = strdup(nr->house);
	member->zip = strdup(nr->zip);
	member->city = strdup(nr->city);
	member->email = strdup(nr->email);
	member->formation = fid;
	add_member(member);

	return MHD_YES;
}

struct member_form_state {
	int pos; /* state machine */
	int fid; /* selector */
	const struct member_struct *m; /* member iterator */
	const struct formation_struct *f; /* formation iterator */
	struct Request *request;
};


/*
    pos:
			0 static
			1 formation
			2 static
			3 member
			4 static
			5 done
*/
static ssize_t member_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct member_form_state *mfs = cls;
	char *p = buf;
	struct user_struct *user = mfs->request->session->logged_in;

	switch (mfs->pos) {
	case 0: 
		p = add_header(p, "Mitglieder");
		p = stpcpy(p, "<div id=\"main\"><form action=\"/member\" method=\"POST\"><p><select name=\"fid\">");
		mfs->pos++;
		mfs->f = get_formation_list();
		return p - buf;
		
	case 1:
		while (mfs->f) {
			if (in_formation(mfs->f->fid, user->fid)) {
				p += sprintf(p, "<option %svalue=\"%d\">%s</option>", 
					mfs->fid == mfs->f->fid ? "selected ":"", mfs->f->fid, mfs->f->name);
				mfs->f = mfs->f->next;
				return p - buf;
			}
			mfs->f = mfs->f->next;
		}
		mfs->pos++;
		/* no break */

	case 2:
		p = stpcpy(p, "</select><input type=\"submit\" value=\"Abrufen\"></p></form>" \
			"<table><tr><th>Vorname</th><th>Nachname</th><th>Straße</th><th>Haus-Nr.</th>" \
			"<th>PLZ</th><th>Ort</th><th>Gliederung</th></tr>");
		mfs->pos++;
		mfs->m = member_list;
		return p - buf;
	
	case 3:
		while (mfs->m) {
			if (in_formation(mfs->m->formation, mfs->fid)) {
				p = stpcpy(p, "<tr><td>");
				p = stpcpy(p, mfs->m->first);
				p = stpcpy(p, "</td><td>");
				p = stpcpy(p, mfs->m->second);
				p = stpcpy(p, "</td><td>");
				p = stpcpy(p, mfs->m->street);
				p = stpcpy(p, "</td><td>");
				p = stpcpy(p, mfs->m->house);
				p = stpcpy(p, "</td><td>");
				p = stpcpy(p, mfs->m->zip);
				p = stpcpy(p, "</td><td>");
				p = stpcpy(p, mfs->m->city);
				p = stpcpy(p, "</td><td>");
				p = stpcpy(p, get_formation(mfs->m->formation)->name);
				p = stpcpy(p, "</td></tr>");
		
				mfs->m = mfs->m->next;
				return p - buf;
			}
			mfs->m = mfs->m->next;
		} 
		mfs->pos++;
		/* no break */

	case 4:
		p = stpcpy(p, "</table></div>");
		p = add_footer(p);
		mfs->pos++;
		return p - buf;

	case 5:	
	default:
		return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
	}
}

struct MHD_Response *member_form(struct Request *request)
{
	struct member_form_state *mfs;

	if (!(mfs = (struct member_form_state *)calloc(1, sizeof(struct member_form_state))))
		return NULL;

	mfs->fid = request->session->logged_in->fid;

	if (request->data) {
		struct MemberRequest *mr = request->data;
		parse_fid(mr->fid, &mfs->fid);
	}
	mfs->request = request;

	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &member_form_reader, mfs, &free);
}
struct newmember_form_state {
	int pos; /* state machine */
	int fid; /* selector */
	const struct formation_struct *f; /* formation iterator */
};

/*
    pos:
			0 static
			1 formation
			2 static
			3 done
*/
static ssize_t newmember_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct newmember_form_state *nfs = cls;
	char *p = buf;

	switch (nfs->pos) {
	case 0:
		p = add_header(p, "Neues Mitglied");
		p = stpcpy(p, "<div id=\"main\">" \
				"<form action=\"/newmember\" method=\"POST\">" \
				"<table>" \
				"<tr><td>Vorname</td><td><input name=\"first\" type=\"text\"></td></tr>" \
				"<tr><td>Nachname</td><td><input name=\"second\" type=\"text\"></td></tr>" \
				"<tr><td>Straße</td><td><input name=\"street\" type=\"text\"></td></tr>" \
				"<tr><td>Haus-Nr.</td><td><input name=\"house\" type=\"text\"></td></tr>" \
				"<tr><td>PLZ</td><td><input name=\"zip\" type=\"text\"></td></tr>" \
				"<tr><td>Ort</td><td><input name=\"city\" type=\"text\"></td></tr>" \
				"<tr><td>Email</td><td><input name=\"email\" type=\"text\"></td></tr>"
				"<tr><td>Gliederung</td><td><select name=\"fid\">");
		nfs->pos++;
		nfs->f = get_formation_list();
		return p - buf;
		
	case 1:
		while (nfs->f) {
			if (in_formation(nfs->f->fid, nfs->fid)) {
				p += sprintf(p, "<option value=\"%d\">%s</option>", nfs->f->fid, nfs->f->name);
				nfs->f = nfs->f->next;
				return p - buf;
			}
			nfs->f = nfs->f->next;
		}
		nfs->pos++;
		/* no break */

	case 2:
		p = stpcpy(p, "</select></td></tr></table>" \
			"<input type=\"submit\" value=\"Hinzufügen\">" \
			"</form></div>");
		p = add_footer(p);
		nfs->pos++;
		return p - buf;

	default:
		return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
	}
}

struct MHD_Response *newmember_form(struct Request *request)
{
	struct newmember_form_state *nfs;

	if (!(nfs = (struct newmember_form_state *)calloc(1, sizeof(struct newmember_form_state))))
		return NULL;

	nfs->fid = request->session->logged_in->fid;

	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &newmember_form_reader, nfs, &free);
}

int member_count(int fid)
{
	int i = 0;
	const struct member_struct *m;

	for (m = member_list; m; m = m->next)
		if (in_formation(m->formation, fid))
			i++;
	return i;
}
