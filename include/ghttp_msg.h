#ifndef _ghttp_msg_h
#define _ghttp_msg_h

#include <stddef.h>
#include <stdbool.h>

enum {
	GHTTP__METHOD_GET = 0,
};

#define general_headers \
	add_header(content_type, "Content-Type", char*, NULL) \
	add_header(content_length, "Content-Length", size_t, "%zu")

#define request_headers \
	add_header(host, "Host", char*, NULL)

#define responce_headers \
	add_header(server, "Server", char*, NULL)

struct ghttp__header {
	char* name;
	char* value;
};

struct ghttp__general_headers {
#	define add_header(x, y, z, w) struct ghttp__header x; z x##_data;
	general_headers
#	undef add_header
};

struct ghttp__request_headers {
#	define add_header(x, y, z, w) struct ghttp__header x; z x##_data;
	request_headers
#	undef add_header
	
	struct ghttp__general_headers general;
};

struct ghttp__responce_headers {
#	define add_header(x, y, z, w) struct ghttp__header x; z x##_data;
	responce_headers
#	undef add_header
	
	struct ghttp__general_headers general;
};

struct ghttp__responce {
	int responce_code;
	char* responce_str;

	struct ghttp__responce_headers headers;

	void* content;
};

struct ghttp__request {
	char* type;
	char* url;

	struct ghttp__request_headers headers;

	void* content;
};

void ghttp__init_msg(void);

bool ghttp__parse_request(struct ghttp__request* request, const char* str);
bool ghttp__parse_responce(struct ghttp__responce* responce, const char* str);

void ghttp__create_request(const struct ghttp__request* request, char* str, size_t* out_size);
void ghttp__create_responce(const struct ghttp__responce* responce, char* str, size_t* out_size);

void ghttp__free_request(const struct ghttp__request* request);
void ghttp__free_responce(const struct ghttp__responce* responce);

#endif
