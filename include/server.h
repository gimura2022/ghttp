#ifndef _server_h
#define _server_h

#include <stddef.h>

#include <gstd/strref.h>

#include <ghttp/messanges.h>

struct ghttp__simple_request {
	struct gstd__strref url;	

	size_t data_size;
	void* data;

	struct ghttp__request* real;
};

struct ghttp__simple_responce {
	int code;
	char* content_type;

	size_t data_size;
	void* data;

	struct ghttp__responce* real;
};

typedef void (*ghttp__responder_process_f)(const struct ghttp__simple_request*,
		struct ghttp__simple_responce*);
typedef bool (*ghttp__responder_checker_f)(const char* url);
typedef void (*ghttp__responder_destructor_f)(struct ghttp__simple_responce*);

struct ghttp__responder {
	char* url;

	ghttp__responder_checker_f checker_func;
	ghttp__responder_process_f process_func;
	ghttp__responder_destructor_f destructor_func;
};

void ghttp__start_server(struct ghttp__responder* responders, size_t responders_count,
		struct ghttp__responder* not_found, int port);

#endif
