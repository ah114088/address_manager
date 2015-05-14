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

#if 0
struct MHD_Response *member_form(struct Request *request)
{	
	const char *page = "<!DOCTYPE html><html><body><p>Hello world!</p></body></html>";
	return MHD_create_response_from_buffer(strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);
}
#else

#define MAXPAGESIZE 1024
struct member_form_state {
	int pos;
};

char *cpy_umlaut(char *dst, const char *str)
{
	static const struct {
		char utf;
		const char *subst;
	} map[] = {
		{ 0xAE, "&Auml;" },
		{ 0x96, "&Ouml;" },
		{ 0x9C, "&Uuml;" },
		{ 0xA4, "&auml;" },
		{ 0xB6, "&ouml;" },
		{ 0xBC, "&uuml;" },
		{ 0x9F, "&szlig;" },
	};
	int i;
	char *p = dst;
	for (i=0; str[i]; i++) {
		if (str[i] == 0xC3) {
			int j;
			for (j=0; j<sizeof(map)/sizeof(map[0]); j++)
				if (str[i+1] == map[j].utf) {
					p = stpcpy(p, map[j].subst);
					i++;
					break;
				}
			if (j == sizeof(map)/sizeof(map[0])) {
				fprintf(stderr, "UTF-8 character %hhx\n!!", str[i+1]);
			}
		} else {
		  *p++ = str[i];
		}
	}
	return p;
}

static ssize_t member_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	int len;
	const char *head = "<!DOCTYPE html><html><body><table><tr><th>Vorname</th><th>Nachname</th><th>Strasse</th><th>Haus-Nr.</th><th>PLZ</th><th>Ort</th></tr>";
	const char *tail = "</table></body></html>";
	struct member_form_state *mfs = cls;
	switch (mfs->pos) {
	case 0:
		len = strlen(head);
		memcpy(buf, head, len);
		mfs->pos++;
		return len;

	default:
		if (mfs->pos < nmember + 1) {
			const struct member_struct *m = &member[mfs->pos-1];
			char *p = buf;
      p = stpcpy(p, "<tr><td>");
			p = cpy_umlaut(p, m->first);
      p = stpcpy(p, "</td><td>");
			p = cpy_umlaut(p, m->second);
      p = stpcpy(p, "</td><td>");
			p = cpy_umlaut(p, m->street);
      p = stpcpy(p, "</td><td>");
			p = cpy_umlaut(p, m->house);
      p = stpcpy(p, "</td><td>");
			p = cpy_umlaut(p, m->zip);
      p = stpcpy(p, "</td><td>");
			p = cpy_umlaut(p, m->city);
      p = stpcpy(p, "</td></tr>");
			mfs->pos++;
			return p - buf;
		} else if (mfs->pos == nmember + 1) {
			len = strlen(tail);
			memcpy(buf, tail, len);
			mfs->pos++;
			return len;
		} else /* nmember + 2 */ {
			return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
		}
	}	
}

static void member_form_free(void *cls)
{
	free(cls);
}

struct MHD_Response *member_form(struct Request *request)
{
	struct member_form_state *mfs;
	if (!(mfs = (struct member_form_state *)calloc(1, sizeof(struct member_form_state))))
		return NULL;
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &member_form_reader, mfs, &member_form_free);
}
#endif
