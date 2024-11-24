#include <string.h>
#include <stdio.h>

#include "ghttp_msg.h"

#define BRBN "\r\n"

bool ghttp__parse_request(struct ghttp__request* request, const char* str);
bool ghttp__parse_responce(struct ghttp__responce* responce, const char* str);

static void add_general_headers(const struct ghttp__general_headers* headers, char* str);
static void add_request_headers(const struct ghttp__request_headers* headers, char* str);
static void add_responce_headers(const struct ghttp__responce_headers* headers, char* str);
static void add_content(const void* content, char* str, size_t* size,
		const struct ghttp__header* content_size);

void ghttp__create_request(const struct ghttp__request* request, char* str, size_t* out_size)
{
	sprintf(str, "%s %s HTTP/1.1" BRBN, request->type, request->url);
	add_general_headers(&request->headers.general, str);
	add_request_headers(&request->headers, str);
	add_content(request->content, str, out_size, &request->headers.general.content_size);
}

void ghttp__create_responce(const struct ghttp__responce* responce, char* str, size_t* out_size)
{
	sprintf(str, "HTTP/1.1 %i %s" BRBN, responce->responce_code, responce->responce_str);
	add_general_headers(&responce->headers.general, str);
	add_responce_headers(&responce->headers, str);
	add_content(responce->content, str, out_size, &responce->headers.general.content_size);
}

static void add_header_to_buf(const char* name, const struct ghttp__header* header, char* str);

static void add_general_headers(const struct ghttp__general_headers* headers, char* str)
{
#	define add_header(x, y) add_header_to_buf(y, &headers.x, str);
	general_headers
#	undef add_header
}

static void add_request_headers(const struct ghttp__request_headers* headers, char* str)
{
#	define add_header(x, y) add_header_to_buf(y, &headers.x, str);
	request_headers
#	undef add_header
}

static void add_responce_headers(const struct ghttp__responce_headers* headers, char* str)
{
#	define add_header(x, y) add_header_to_buf(y, &headers.x, str);
	responce_headers
#	undef add_header
}

static void add_header_to_buf(const char* name, const struct ghttp__header* header, char* str)
{
	if (header->value != NULL) {
		strcat(str, name);
		strcat(str, ": ");
		strcat(str, header->value);
		strcat(str, BRBN);
	}
}

static void add_content(const void* content, char* str, size_t* size,
		const struct ghttp__header* content_size)
{
	strcat(str, BRBN);
	*size = 0;
	*size += strlen(str);

	if (content == NULL || content_size->value == NULL) return;

	size_t content_size_int = 0;
	if (sscanf(content_size->value, "%zu", &content_size_int) != 1) return;

	memcpy(str + *size, content, content_size_int);
	*size += content_size_int;
}
