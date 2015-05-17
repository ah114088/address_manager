#ifndef __FORMATION_H
#define __FORMATION_H

struct formation_struct {
	struct formation_struct *next;
	char *name;
	int fid;
	struct formation_struct *super;
};

int parse_fid(const char *fid_str, int *fid);
int init_formation_list(void);
int in_formation(int member_formation, int fid);

struct MHD_Response *formation_form(struct Request *request);
struct MHD_Response *newformation_form(struct Request *request);

struct formation_struct *get_formation(int fid);
const struct formation_struct *get_formation_list(void);

struct NewFormRequest {
  char name[MAX_STR];
  char fid[6];
};
int newform_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size);
int newform_process(struct Request *request);

#endif
