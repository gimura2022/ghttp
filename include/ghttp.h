#ifndef _http_server_h
#define _http_server_h

#include <stdlib.h>
#include <stdbool.h>

#define GHTTP_MAX_NOT_PARSED_HEADERS 128

struct ghttp__header {
	const char* name;
	const char* value;
	void* value_converted;
};

struct ghttp__general_headers {
	struct ghttp__header accept;
	struct ghttp__header accept_ch;
	struct ghttp__header accept_charset;
	struct ghttp__header accept_encoding;
	struct ghttp__header accept_language;
	struct ghttp__header accept_ranges;
	struct ghttp__header connection;	
	struct ghttp__header content_encoding;
	struct ghttp__header content_language;
	struct ghttp__header content_lenght;
	struct ghttp__header content_location;
	struct ghttp__header content_range;
	struct ghttp__header content_type;
	struct ghttp__header date;
	struct ghttp__header pragma;
	struct ghttp__header transfer_encoding;
	struct ghttp__header user_agent;
	struct ghttp__header vary;
	struct ghttp__header via;
	struct ghttp__header warning;
};

struct ghttp__responce_headers {
	struct ghttp__general_headers general_headers;

	struct ghttp__header age;
	struct ghttp__header allow;
	struct ghttp__header location;
	struct ghttp__header proxy_authenticate;
	struct ghttp__header range;
	struct ghttp__header server;
	struct ghttp__header www_authenticate;
};

struct ghttp__request_headers {
	struct ghttp__general_headers general_headers;

	struct ghttp__header autorisation;
	struct ghttp__header expect;
	struct ghttp__header from;
	struct ghttp__header host;
	struct ghttp__header if_match;
	struct ghttp__header if_range;
	struct ghttp__header if_unmodified_since;
	struct ghttp__header max_forwards;
	struct ghttp__header proxy_authorization;
	struct ghttp__header referer;
	struct ghttp__header retry_after;
	struct ghttp__header tf;
	struct ghttp__header upgrade;
};

struct ghttp__request {
	const char* method;
	const char* url;

	struct ghttp__request_headers request_headers;
	struct ghttp__header not_parssed_headers[GHTTP_MAX_NOT_PARSED_HEADERS];
	size_t not_parssed_headers_count;
};

struct ghttp__responce {
	int code;
	const char* msg;

	struct ghttp__responce_headers responce_headers;
	struct ghttp__header not_parssed_headers[GHTTP_MAX_NOT_PARSED_HEADERS];
	size_t not_parssed_headers_count;
};

void ghttp__create_responce(char* buf, const size_t buf_size, struct ghttp__responce* responce);
void ghttp__create_request(char* buf, const size_t buf_size, struct ghttp__request* request);

bool ghttp__parse_responce(const char* buf, const size_t buf_size, struct ghttp__responce* responce);
bool ghttp__parse_request(const char *buf, const size_t buf_size, struct ghttp__request* request);

#endif
