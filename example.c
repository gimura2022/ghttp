#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "include/ghttp.h"

static struct ghttp__responce handler(struct ghttp__server_context* ctx, struct ghttp__request request)
{
	printf("Prinal\n");

	struct ghttp__responce responce = {0};
	responce.responce_code = 200;
	strcpy(responce.responce_str, "OK");

	responce.content.h = true;
	strcpy(responce.content.d.content_type, "text/html");

	strcpy(responce.content.d.content, "Hello, world!");
	responce.content.d.content_size = strlen(responce.content.d.content);

	strcpy(responce.server, "Gimura Server");

	return responce;
}

int main(int argc, char* argv[])
{
	ghttp__start_server(ghttp__get_default_server_data(), &(struct ghttp__path_responder) {
				.path = "/",
				.responder = handler,
				.request_type = HTTPRQT_GET,
				.use_regex = false
			}, 1, NULL);	

	return 0;
}
