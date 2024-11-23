#include <stdio.h>
#include <string.h>

#include "ghttp.h"

#define BRBN "\r\n"

static void ghttp__create_responce_headers(char* buf, const struct ghttp__responce_headers* headers);

void ghttp__create_responce(char* buf, const size_t buf_size, const struct ghttp__responce* responce)
{
	char temp[64] = {0};

	memset(buf, '\0', buf_size);

	strcat(buf, "HTTP/1.1 ");
	
	sprintf(buf, "%i", responce.code);
	strcat(buf, temp);
	strcat(buf, " ");

	strcat(buf, responce.msg);
	strcat(buf, BRBN);

	ghttp__create_responce_headers(buf, &responce->responce_headers);
}

void ghttp__create_request(char* buf, const size_t buf_size, const struct ghttp__request* request);

bool ghttp__parse_responce(const char* buf, const size_t buf_size, struct ghttp__responce* responce);
bool ghttp__parse_request(const char *buf, const size_t buf_size, struct ghttp__request* request);

static const char* age_name                = "Age";
static const char* allow_name              = "Allow";
static const char* location_name           = "Location";
static const char* proxy_authenticate_name = "Proxy-Authenticate";
static const char* range_name              = "Range";
static const char* server_name             = "Server";
static const char* www_authenticate_name   = "WWW-Authenticate";

static void ghttp__add_header(char* buf, const struct ghttp__header* header);

static void ghttp__create_responce_headers(char* buf, const struct ghttp__responce_headers* headers)
{
	if (headers->age.name == NULL) headers->age.name = age_name;
	if (headers->allow.name == NULL) headers->allow.name = allow_name;
	if (headers->location.name == NULL) headers->location.name = location_name;
	if (headers->proxy_authenticate.name == NULL) headers->proxy_authenticate.name = proxy_authenticate_name;
	if (headers->range.name == NULL) headers->range.name = range_name;
	if (headers->server.name == NULL) headers->server.name = server_name;
	if (headers->www_authenticate.name == NULL) headers->www_authenticate.name = www_authenticate_name;

	ghttp__add_header(buf, &headers->age);
	ghttp__add_header(buf, &headers->allow);
	ghttp__add_header(buf, &headers->location);
	ghttp__add_header(buf, &headers->proxy_authenticate);
	ghttp__add_header(buf, &headers->range);
	ghttp__add_header(buf, &headers->server);
	ghttp__add_header(buf, &headers->www_authenticate);
}

static void ghttp__add_header(char* buf, const struct ghttp__header* header)
{
	if (header.value == NULL) return;

	strcat(buf, header.name);
	strcat(buf, ": ");
	strcat(buf, header.value);
	strcat(buf, BRBN);
}
