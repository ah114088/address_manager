#ifndef __MEMBER_H
#define __MEMBER_H

struct MemberRequest { 
  char fid[6];
};

int member_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size);
int member_process(struct Request *request);

struct NewMemberRequest { 
	char first[MAX_STR];
	char second[MAX_STR];
	char street[MAX_STR];
	char house[MAX_STR];
	char zip[MAX_STR];
	char city[MAX_STR];
	char email[MAX_STR];
  char fid[6];
};

int newmember_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size);
int newmember_process(struct Request *request);

int member_count(int fid);

struct MHD_Response *member_form(struct Request *request);
struct MHD_Response *newmember_form(struct Request *request);

#endif
