#ifndef __MEMBER_H
#define __MEMBER_H

int read_member_list(const char *file);
struct MHD_Response *member_form(struct Request *request);

#endif
