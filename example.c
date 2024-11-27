#include <ghttp.h>
#include <ghttp_msg.h>
#include <glog.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/ghttp.h"

static struct ghttp__responce handler(struct ghttp__server_context* ctx, struct ghttp__request* request)
{
	printf("Prinal\n");

	struct ghttp__responce responce = {0};
	responce.responce_code = 200;
	responce.content = malloc(100);
	memset(responce.content, '\0', 100);
	strcpy(responce.content, "Hello, world!");

	responce.responce_str = malloc(100);
	strcpy(responce.responce_str, "OK");

	responce.headers.server.value = malloc(100);
	strcpy(responce.headers.server.value, "Gimura server");

	responce.headers.general.content_length.value = malloc(100);
	strcpy(responce.headers.general.content_length.value, "100");

	responce.headers.general.content_type.value = malloc(100);
	strcpy(responce.headers.general.content_type.value, "text/html");

	return responce;
}

int main(int argc, char* argv[])
{
	struct glog__logger http_logger = {0};
	glog__logger_from_prefix(&http_logger, "ghttp");

	ghttp__init(malloc, free, &http_logger);

	ghttp__start_server((struct ghttp__server_data) {
				.port = 8080
			}, &(struct ghttp__path_responder) {
				.path         = "/",
				.responder    = handler,
				.request_type = "GET",
				.use_regex    = false,
			}, 1, NULL);

	return 0;
}
