#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <ghttp/parse.h>
#include <ghttp/ghttp.h>
#include <ghttp/messanges.h>
#include <ghttp/gen.h>
#include <ghttp/send.h>
#include <ghttp/server.h>

#include <glog.h>

#include <gstd/allocators.h>
#include <gstd/strref.h>
#include <gstd/utils.h>

static const char* out_content_type = "text/html";
static const char* out_content      = "Hello, world!";

static void handler_process(const struct ghttp__simple_request* request,
		struct ghttp__simple_responce* responce)
{
	responce->code         = 200;
	responce->content_type = (char*) out_content_type;
	responce->data         = (char*) out_content;
	responce->data_size    = strlen(out_content);
}

static void handler_destructor(struct ghttp__simple_responce* responce) {}

static void handler_not_found(const struct ghttp__simple_request* request,
		struct ghttp__simple_responce* responce)
{
	responce->code = 404;
	responce->data = NULL;
}

int main(int argv, char* argc[])
{
	struct glog__logger logger = {0};
	struct gstd__memmanager memmanager = (struct gstd__memmanager) {
		.allocator   = malloc,
		.deallocator = free,
	};

	glog__init();
	glog__logger_from_prefix(&logger, "ghttp");

	ghttp__init(&memmanager, &logger);

	struct ghttp__responder responders[] = {
		(struct ghttp__responder) {
			.url       = "/",
			.use_regex = false,

			.process_func    = handler_process,
			.destructor_func = handler_destructor,
		},
	};
	ghttp__start_server(responders, array_size(responders), &(struct ghttp__responder) {
				.destructor_func = handler_destructor,
				.process_func    = handler_not_found,
			}, 8080);

	return 0;
}
