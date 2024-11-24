#ifndef _ghttp_msg_h
#define _ghttp_msg_h

#include <stddef.h>
#include <stdbool.h>

#define general_headers \
	add_headers(content_type, "Content-Type") \
	add_headers(content_length, "Content-Length")

#define request_headers \
	add_headers(host, "Host")

#define responce_headers \
	add_headers(server, "Server")

struct ghttp__header {
	char* name;
	char* value;
};

struct ghttp__general_headers {
#	define add_headers(x, y) struct ghttp__header x;
	general_headers
#	undef add_headers
};

struct ghttp__request_headers {
#	define add_headers(x, y) struct ghttp__header x;
	request_headers
#	undef add_headers
	
	struct ghttp__general_headers general;
};

struct ghttp__responce_headers {
#	define add_headers(x, y) struct ghttp__header x;
	responce_headers
#	undef add_headers
	
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

bool ghttp__parse_request(struct ghttp__request* request, const char* str);
bool ghttp__parse_responce(struct ghttp__responce* responce, const char* str);

void ghttp__create_request(const struct ghttp__request* request, char* str, size_t* out_size);
void ghttp__create_responce(const struct ghttp__responce* responce, char* str, size_t* out_size);

#endif
