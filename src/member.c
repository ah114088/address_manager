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
};

#define MAX_MEMBER 100
#define MAX_STR 100
struct member_struct member[MAX_MEMBER];
int nmember;

// UTF-8
int read_member_list(const char *file)
{
	FILE *fp;
	char buffer[8][MAX_STR];
	int i = 0;

	if (!(fp=fopen(file, "r"))) 
		return -1;

	while (fscanf(fp, "%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]\n", 
			buffer[0], buffer[1], buffer[2], buffer[3],
			buffer[4], buffer[5], buffer[6], buffer[7]) == 8) {
		if (i == MAX_MEMBER) {
			fprintf(stderr, "too many members!!\n");
			return -1;
		}
		member[i].first   = strdup(buffer[0]);
		member[i].second  = strdup(buffer[1]);
		member[i].street  = strdup(buffer[2]);
		member[i].house   = strdup(buffer[3]);
		member[i].zip     = strdup(buffer[4]);
		member[i].city    = strdup(buffer[5]);
		member[i].country = strdup(buffer[6]);
		member[i].email   = strdup(buffer[7]);
		fprintf(stderr, "read: %s %s\n", member[i].first, member[i].second);
		i++;
	}
		
	fclose(fp);

	nmember = i;
	return i;
}

#define MAXPAGESIZE 1024
struct member_form_state {
	int pos;
};

static ssize_t member_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct member_form_state *mfs = cls;
	char *p = buf;

	switch (mfs->pos) {
	case 0:
		p = stpcpy(p, "<!DOCTYPE html><html><head><title>Mitglieder</title>" \
				"<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">" \
			"</head><body><div id=\"main\"><table><tr><th>Vorname</th><th>Nachname</th><th>Strasse</th><th>Haus-Nr.</th><th>PLZ</th><th>Ort</th></tr>");
		mfs->pos++;
		break;

	default:
		if (mfs->pos < nmember + 1) {
			const struct member_struct *m = &member[mfs->pos-1];
      p = stpcpy(p, "<tr><td>");
			p = stpcpy(p, m->first);
      p = stpcpy(p, "</td><td>");
			p = stpcpy(p, m->second);
      p = stpcpy(p, "</td><td>");
			p = stpcpy(p, m->street);
      p = stpcpy(p, "</td><td>");
			p = stpcpy(p, m->house);
      p = stpcpy(p, "</td><td>");
			p = stpcpy(p, m->zip);
      p = stpcpy(p, "</td><td>");
			p = stpcpy(p, m->city);
      p = stpcpy(p, "</td></tr>");
			mfs->pos++;
			break;
			
		} else if (mfs->pos == nmember + 1) {
			p = stpcpy(p, "</table></div></body></html>");
			mfs->pos++;
			break;
		} else /* nmember + 2 */ {
			return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
		}
	}	

	return p - buf;
}

struct MHD_Response *member_form(struct Request *request)
{
	struct member_form_state *mfs;
	if (!(mfs = (struct member_form_state *)calloc(1, sizeof(struct member_form_state))))
		return NULL;
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &member_form_reader, mfs, &free);
}
