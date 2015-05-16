#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#include "request.h"
#include "member.h"
#include "user.h"
#include "formation.h"

struct member_struct;
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

// UTF-8
int read_member_list(const char *path)
{
	FILE *file;
	char buffer[8][MAX_STR];
	int i = 0;
	struct member_struct *m, **mp = &member_list; 

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
		m->next = NULL;
		if (sscanf(buffer[7], "%d", &m->formation)!=1) {
			fprintf(stderr, "bad formation number!!\n");
			fclose(file);
			return -1;
		}
		*mp = m;
		mp = &m->next;

		/* fprintf(stderr, "read: %s %s\n", m->first, m->second); */
		i++;
	}
		
	fclose(file);

	nmember = i;
	return i;
}

int member_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
	struct MemberRequest *mr = (struct MemberRequest *)request->data;

  if (!strcmp("fid", key)) {
		to_str(off, size, sizeof(mr->fid), data, mr->fid);		
		return MHD_YES;
	}
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

int member_count(int fid)
{
	int i = 0;
	const struct member_struct *m;

	for (m = member_list; m; m = m->next)
		if (in_formation(m->formation, fid))
			i++;
	return i;
}
