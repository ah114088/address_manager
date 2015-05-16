#ifndef __MEMBER_H
#define __MEMBER_H

struct MemberRequest { 
  char fid[6];
};

int read_member_list(const char *file);
int member_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size);
struct MHD_Response *member_form(struct Request *request);
int member_count(int fid);

#endif
