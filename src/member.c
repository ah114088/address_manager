#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#include "request.h"
#include "member.h"

#define MAX_STR 100

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

struct formation_struct;
struct formation_struct {
	struct formation_struct *next;
	char *name;
	int fid;
	struct formation_struct *super;
};

static struct formation_struct *formation_list = NULL;
static int nformation;

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

static struct formation_struct *get_formation(int fid)
{
	struct formation_struct *f = formation_list;
	for (f = formation_list; f && f->fid != fid; f = f->next)
		;
	return f;
}

int read_formation_list(const char *path) 
{
	FILE *file;
	char buffer[3][MAX_STR];
	int super, i = 0;
	struct formation_struct *f, **fp = &formation_list;

	if (!(file=fopen(path, "r"))) 
		return -1;

	while (fscanf(file, "%[^\t]\t%[^\t]\t%[^\n]\n", buffer[0], buffer[1], buffer[2]) == 3) {
		if (!(f = (struct formation_struct *)malloc(sizeof(struct formation_struct)))) { 
			fprintf(stderr, "malloc() failure!!\n");
			fclose(file);
			return -1;
		}
		f->name = strdup(buffer[1]);
		f->fid = i;
		f->next = NULL;

		if (sscanf(buffer[2], "%d", &super)!=1) {
			fprintf(stderr, "bad super formation!!\n");
			fclose(file);
			return -1;
		}

		if (!(f->super = get_formation(super)) && i != 0) {
			fprintf(stderr, "no formation with fid %d!!\n", super);
			fclose(file);
			return -1;
		}

		*fp = f;
		fp = &f->next;
		/* fprintf(stderr, "read: %s %d\n", f->name, f->fid); */
		i++;
	}
		
	fclose(file);

	nformation = i;
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

static int in_formation(int member_formation, int fid)
{
	struct formation_struct *f = get_formation(member_formation);
	while (f) {
		if (f->fid == fid)
			return 1;
		f = f->super;
	}
	return 0;
}

struct member_form_state {
	int pos; /* state machine */
	int fid; /* selector */
	const struct member_struct *m; /* member iterator */
	const struct formation_struct *f; /* formation iterator */
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

	switch (mfs->pos) {
	case 0: 
		p = add_header(p, "Mitglieder");
		p = stpcpy(p, "<div id=\"main\"><form action=\"/member\" method=\"POST\"><p><select name=\"fid\">");
		mfs->pos++;
		mfs->f = formation_list;
		return p - buf;
		
	case 1:
		if (mfs->f) {
			p += sprintf(p, "<option %svalue=\"%d\">%s</option>", mfs->fid == mfs->f->fid ? "selected ":"", mfs->f->fid, mfs->f->name);
			mfs->f = mfs->f->next;
			return p - buf;
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

	if (request->data) {
		struct MemberRequest *mr = request->data;
		if (sscanf(mr->fid, "%d", &mfs->fid) != 1 || mfs->fid >= nformation|| mfs->fid < 0)
			fprintf(stderr, "invalid fid: %s\n", mr->fid);
	}
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &member_form_reader, mfs, &free);
}

struct formation_form_state {
	int pos;
	const struct formation_struct *f; /* formation iterator */
};

/*
    pos:
			0 static
			1 formation
			2 static
			3 done
*/
static ssize_t formation_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct formation_form_state *ffs = cls;
	char *p = buf;

	switch (ffs->pos) {
	case 0:
		p = add_header(p, "Gliederungen");
		p = stpcpy(p, "<body><div id=\"main\"><table><tr><th>ID</th><th>Gliederung</th><th>Übergliederung</th></tr>");
		ffs->pos++;
		ffs->f = formation_list;
		return p - buf;

	case 1:
		if (ffs->f) {
      p = stpcpy(p, "<tr><td>");
			p += sprintf(p, "%d", ffs->f->fid);
      p = stpcpy(p, "</td><td>");
			p = stpcpy(p, ffs->f->name);
      p = stpcpy(p, "</td><td>");
			p += sprintf(p, "%d",  ffs->f->super ? ffs->f->super->fid : -1);
      p = stpcpy(p, "</td></tr>");
			ffs->f = ffs->f->next;
			return p - buf;
		} 
		ffs->pos++;
		/* no break */

	case 2:
			p = stpcpy(p, "</table></div>");
			p = add_footer(p);
			ffs->pos++;
			return p - buf;
	
	case 3:
	default:
		return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
	}	

	return p - buf;
}

struct MHD_Response *formation_form(struct Request *request)
{
	struct formation_form_state *mfs;
	if (!(mfs = (struct formation_form_state *)calloc(1, sizeof(struct formation_form_state))))
		return NULL;
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &formation_form_reader, mfs, &free);
}
