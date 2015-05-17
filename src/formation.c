#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#include "request.h"
#include "user.h"
#include "formation.h"
#include "member.h"

static struct formation_struct *formation_list = NULL;
static int nformation;

const struct formation_struct *get_formation_list(void)
{
	return formation_list;
}

static void add_formation(struct formation_struct *new)
{
	int max_fid = 0;

	struct formation_struct **fp;
	for (fp = &formation_list; *fp; fp = &(*fp)->next)
		if ((*fp)->fid > max_fid)
			max_fid = (*fp)->fid;
	new->next = NULL;
	new->fid = max_fid + 1;
	*fp = new;
	nformation++;
}

struct formation_struct *get_formation(int fid)
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

int in_formation(int member_formation, int fid)
{
	struct formation_struct *f = get_formation(member_formation);
	while (f) {
		if (f->fid == fid)
			return 1;
		f = f->super;
	}
	return 0;
}

int parse_fid(const char *fid_str, int *fid)
{
	if (sscanf(fid_str, "%d", fid) != 1 || *fid >= nformation|| *fid < 0) {
		fprintf(stderr, "invalid fid: %s\n", fid_str);
		return -1;
	}
	return 0;
}

struct formation_form_state {
	int pos;
	const struct formation_struct *f; /* formation iterator */
	int fid; /* selector */
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
		p = stpcpy(p, "<div id=\"main\"><table><tr><th>Gliederung</th><th>Mitglieder</th></tr>");
		ffs->pos++;
		ffs->f = formation_list;
		return p - buf;

	case 1:
		while (ffs->f) {
			if (in_formation(ffs->f->fid, ffs->fid)) {
				p = stpcpy(p, "<tr><td>");
				p = stpcpy(p, ffs->f->name);
				p = stpcpy(p, "</td><td>");
				p += sprintf(p, "%d", member_count(ffs->f->fid));
				p = stpcpy(p, "</td></tr>");
				ffs->f = ffs->f->next;
				return p - buf;
			}
			ffs->f = ffs->f->next;
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
	mfs->fid = request->session->logged_in->fid;
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &formation_form_reader, mfs, &free);
}

struct newformation_form_state {
	int pos;
	const struct formation_struct *f; /* formation iterator */
	int fid; /* selector */
};

/*
    pos:
			0 static
			1 formation
			2 static
			3 done
*/
static ssize_t newformation_form_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	struct newformation_form_state *nfs = cls;
	char *p = buf;

	switch (nfs->pos) {
	case 0:
		p = add_header(p, "Gliederung hinzufügen");
		p = stpcpy(p, "<div id=\"main\">" \
				"<form action=\"/newformation\" method=\"POST\">" \
				"<table>" \
				"<tr><td>Name</td><td><input name=\"name\" type=\"text\"></td></tr>" \
				"<tr><td>Übergeordnete Gliederung</td><td><select name=\"fid\">");
		nfs->pos++;
		nfs->f = formation_list;
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
	
	case 3:
	default:
		return MHD_CONTENT_READER_END_OF_STREAM; /* no more bytes */
	}	

	return p - buf;
}

struct MHD_Response *newformation_form(struct Request *request)
{
	struct newformation_form_state *nfs;
	if (!(nfs = (struct newformation_form_state *)calloc(1, sizeof(struct newformation_form_state))))
		return NULL;
	nfs->fid = request->session->logged_in->fid;
	return MHD_create_response_from_callback(-1, MAXPAGESIZE, &newformation_form_reader, nfs, &free);
}

int newform_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size)
{
  struct Request *request = cls;
	struct NewFormRequest *nr = (struct NewFormRequest *)request->data;

	COPY_AND_RETURN(nr, "name", name)
	COPY_AND_RETURN(nr, "fid", fid)
  fprintf(stderr, "Unsupported form value `%s'\n", key);
  return MHD_NO;
}
int newform_process(struct Request *request)
{
	struct formation_struct *f, *super;
	struct NewFormRequest *nr = request->data;
	int super_fid;

	/* TODO: check duplicate formation */

	if (parse_fid(nr->fid, &super_fid)<0 ||!(super = get_formation(super_fid)))
		return MHD_NO;

	if (!(f = (struct formation_struct *)malloc(sizeof(struct formation_struct)))) {
		fprintf(stderr, "malloc() failure!!\n");
		return MHD_NO;
	}

	/* TODO: check data consistency */
	/* TODO: check malloc failure */
	f->name = strdup(nr->name);
	f->super = super;
	add_formation(f);

	fprintf(stderr, "new formation %s with fid %d as subformation of %s\n", f->name, f->fid, f->super->name);

	return MHD_YES;
}
