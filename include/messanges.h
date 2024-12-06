#ifndef _messagnes_h
#define _messagnes_h

#include <stddef.h>
#include <stdbool.h>

#include <gstd/strref.h>

#define GHTTP__BRBN "\r\n"

#define ghttp__process_general_headers \
		add_header(content_type   , "Content-Type") \
		add_header(content_length , "Content-Length") \

#define ghttp__process_request_headers  \
		add_header(host , "Host") \

#define ghttp__process_responce_headers  \
		add_header(server , "Server") \

struct ghttp__header {
	struct gstd__strref name;
	struct gstd__strref value;
	bool has;
};

#define add_header(x, y) struct ghttp__header x;

struct ghttp__general_headers {
	ghttp__process_general_headers
};

struct ghttp__request_headers {
	ghttp__process_request_headers
	struct ghttp__general_headers general;
};

struct ghttp__responce_headers {
	ghttp__process_responce_headers
	struct ghttp__general_headers general;
};

#undef add_header

struct ghttp__responce {
	int code;
	struct ghttp__responce_headers headers;

	size_t data_size;
	void* data;
};

struct ghttp__request_get {
	struct gstd__strref url;
};

struct ghttp__request {
	enum {
		GHTTP__REQUEST_GET = 0,
	} type;

	union {
		struct ghttp__request_get get;
	};

	struct ghttp__request_headers headers;

	size_t data_size;
	void* data;
};

#endif
