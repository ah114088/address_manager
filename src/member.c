#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#include "request.h"
#include "member.h"

struct member_struct {
	char *first;
	char *second;
	char *street;
	char *house;
	char *zip;
	char *city;
	char *country;
	char *email;
	int formation;
};

#define MAX_MEMBER 100
#define MAX_STR 100
struct member_struct member[MAX_MEMBER];
int nmember;

struct formation_struct {
	char *name;
	int super;
};

#define MAX_FORMATION 50
struct formation_struct formation[MAX_FORMATION];
int nformation;

// UTF-8
int read_member_list(const char *file)
{
	FILE *fp;
	char buffer[9][MAX_STR];
	int i = 0;

	if (!(fp=fopen(file, "r"))) 
		return -1;

	while (fscanf(fp, "%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]\n", 
			buffer[0], buffer[1], buffer[2], buffer[3],
			buffer[4], buffer[5], buffer[6], buffer[7], buffer[8]) == 9) {
		if (i == MAX_MEMBER) {
			fprintf(stderr, "too many members!!\n");
			return -1;
		}
		member[i].first     = strdup(buffer[0]);
		member[i].second    = strdup(buffer[1]);
		member[i].street    = strdup(buffer[2]);
		member[i].house     = strdup(buffer[3]);
		member[i].zip       = strdup(buffer[4]);
		member[i].city      = strdup(buffer[5]);
		member[i].country   = strdup(buffer[6]);
		member[i].email     = strdup(buffer[7]);
		if (sscanf(buffer[8], "%d", &member[i].formation)!=1) {
			fprintf(stderr, "bad formation number!!\n");
			return -1;
		}

		/* fprintf(stderr, "read: %s %s\n", member[i].first, member[i].second); */
		i++;
	}
		
	fclose(fp);

	nmember = i;
	return i;
}

int read_formation_list(const char *file) 
{
	FILE *fp;
	char buffer[3][MAX_STR];
	int i = 0;

	if (!(fp=fopen(file, "r"))) 
		return -1;

	while (fscanf(fp, "%[^\t]\t%[^\t]\t%[^\n]\n", buffer[0], buffer[1], buffer[2]) == 3) {
		if (i == MAX_FORMATION) {
			fprintf(stderr, "too many formations!!\n");
			return -1;
		}
		formation[i].name      = strdup(buffer[1]);
		if (sscanf(buffer[2], "%d", &formation[i].super)!=1) {
			fprintf(stderr, "bad super formation!!\n");
			return -1;
		}
		/* fprintf(stderr, "read: %s %d\n", formation[i].name, formation[i].super); */
		i++;
	}
		
	fclose(fp);

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
	while (member_formation >= 0) {
		if (member_formation == fid)
			return 1;
		member_formation = formation[member_formation].super;
	}
	return 0;
}

#define MAXPAGESIZE 1024
struct member_form_state {
	int pos;
	int fid; /* selector */
	const struct member_struct *m;
	int f; /* formation index */
};

/*
			"<form action=\"/member\" method=\"POST\"><p>Gliederung: <input name=\"fid\" type=\"number\">"
			"<input type=\"submit\" value=\"Abrufen\"></p></form>"
*/

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
		mfs->f = 0;
		return p - buf;
		
	case 1:
		if (mfs->f < nformation) {
			const struct formation_struct *f = &formation[mfs->f];
			p += sprintf(p, "<option %svalue=\"%d\">%s</option>", mfs->fid == mfs->f ? "selected ":"", mfs->f, f->name);
			mfs->f++;
			return p - buf;
		}
		mfs->pos++;
		/* no break */

	case 2:
		p = stpcpy(p, "</select><input type=\"submit\" value=\"Abrufen\"></p></form>" \
			"<table><tr><th>Vorname</th><th>Nachname</th><th>Straße</th><th>Haus-Nr.</th>" \
			"<th>PLZ</th><th>Ort</th><th>Gliederung</th></tr>");
		mfs->pos++;
		mfs->m = &member[0];
		return p - buf;
	
	case 3:
		while (mfs->m < &member[nmember]) {
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
				p = stpcpy(p, formation[mfs->m->formation].name);
				p = stpcpy(p, "</td></tr>");
		
				mfs->m++;
				return p - buf;
			}
			mfs->m++;
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
};

static ssize_t formation_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct formation_form_state *ffs = cls;
	char *p = buf;

	switch (ffs->pos) {
	case 0:
		p = add_header(p, "Gliederungen");
		p = stpcpy(p, "<body><div id=\"main\"><table><tr><th>ID</th><th>Gliederung</th><th>Übergliederung</th></tr>");
		ffs->pos++;
		break;

	default:
		if (ffs->pos < nformation + 1) {
			const struct formation_struct *f = &formation[ffs->pos-1];
      p = stpcpy(p, "<tr><td>");
			p += sprintf(p, "%d", ffs->pos - 1);
      p = stpcpy(p, "</td><td>");
			p = stpcpy(p, f->name);
      p = stpcpy(p, "</td><td>");
			p += sprintf(p, "%d", f->super);
      p = stpcpy(p, "</td></tr>");
			ffs->pos++;
			break;
			
		} else if (ffs->pos == nformation + 1) {
			p = stpcpy(p, "</table></div>");
			p = add_footer(p);
			ffs->pos++;
			break;
		} else /* nformation + 2 */ {
			return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
		}
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
