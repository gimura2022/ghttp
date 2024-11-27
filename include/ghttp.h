#ifndef _http_server_h
#define _http_server_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#include <ghttp_msg.h>

struct ghttp__server_context {
	bool runned;	
};

typedef struct ghttp__responce (*ghttp__respoder_t)(struct ghttp__server_context*, struct ghttp__request*);

struct ghttp__path_responder {
	const char* path;
	const char* request_type;
	const ghttp__respoder_t responder;
	const bool use_regex;
};

struct ghttp__server_data {
	uint16_t port;	
};

typedef void* (*ghttp__malloc_t)(size_t);
typedef void (*ghttp__free_t)(void*);

extern ghttp__malloc_t ghttp__malloc;
extern ghttp__free_t ghttp__free;

void ghttp__init(ghttp__malloc_t allocator, ghttp__free_t deallocator, struct glog__logger* logger);

void ghttp__start_server(struct ghttp__server_data server_data, struct ghttp__path_responder* responders,
		size_t responder_count, ghttp__respoder_t not_found_responder);

bool ghttp__send_request(char* host, int port, const struct ghttp__request* request,
		struct ghttp__responce* responce);

#endif
