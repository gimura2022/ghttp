#ifndef _http_server_h
#define _http_server_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#define GHTTP_MAX_URL 1024
#define GHTTP_MAX_HOST 1024
#define GHTTP_MAX_RESPONCE_STR 1024
#define GHTTP_MAX_SERVER 512
#define GHTTP_MAX_CONTENT_TYPE 1024
#define HTTP_BUFFER_SIZE 1024 * 64 



struct ghttp__server_context {
	bool runned;	
};

typedef struct ghttp__responce (*ghttp__respoder_t)(struct ghttp__server_context*, struct ghttp__request);

struct ghttp__path_responder {
	const char* path;
	const ghttp__respoder_t responder;
	const enum ghttp__request_type request_type;
	const bool use_regex;
};

struct ghttp__server_data {
	uint16_t port;	
};

struct ghttp__server_data ghttp__get_default_server_data(void);

void ghttp__start_server(struct ghttp__server_data server_data, struct ghttp__path_responder* responders,
		size_t responder_count, ghttp__respoder_t not_found_responder);

bool ghttp__send_request(const char* host, int port, const struct ghttp__request* request,
		struct ghttp__responce* responce);

#endif
