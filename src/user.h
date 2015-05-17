#ifndef __USER_H
#define __USER_H

#define MAXUSERNAME 32
#define MAXPASSWORD 32

struct user_struct {
	struct user_struct *next;
  char username[MAXUSERNAME+1];
  char password[MAXPASSWORD+1];
	int fid;
};

int init_user_list(void);
struct user_struct *find_user(const char *username);

struct MHD_Response *newuser_form(struct Request *request);
struct MHD_Response *user_form(struct Request *request);
struct MHD_Response *welcome_form(struct Request *request);
struct MHD_Response *chpass_form(struct Request *request);

struct NewuserRequest {
  char username[MAXUSERNAME+1];
  char password[MAXPASSWORD+1];
  char fid[6];
};
int newuser_iterator(void *cls, enum MHD_ValueKind kind, const char *key,
	       const char *filename, const char *content_type, const char *transfer_encoding,
	       const char *data, uint64_t off, size_t size);
int newuser_process(struct Request *request);

#endif
