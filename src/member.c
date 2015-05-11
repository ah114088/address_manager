#include <stdio.h>
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

#define MAX_MEMBER 50
struct member_struct member[MAX_MEMBER];
int nmember;

int read_member_list(const char *file)
{
	FILE *fp;
	char buffer[8][100];
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

struct MHD_Response *member_form(struct Request *request)
{	
	const const char page[] = "<!DOCTYPE html><html><body><p>Hello world!</p></body></html>";
	return MHD_create_response_from_buffer(strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);
}
