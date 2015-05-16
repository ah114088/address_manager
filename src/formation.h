#ifndef __FORMATION_H
#define __FORMATION_H

int parse_fid(const char *fid_str, int *fid);
int read_formation_list(const char *file);
int in_formation(int member_formation, int fid);

struct MHD_Response *formation_form(struct Request *request);

struct formation_struct {
	struct formation_struct *next;
	char *name;
	int fid;
	struct formation_struct *super;
};

struct formation_struct *get_formation(int fid);
const struct formation_struct *get_formation_list(void);

#endif
