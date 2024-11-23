#include <stdio.h>
#include <string.h>

#include "ghttp.h"

#define BRBN "\r\n"

#define set_if_null(x, y) ({ if (x == NULL) x = y; })

static void ghttp__create_responce_headers(char* buf, struct ghttp__responce_headers* headers);

void ghttp__create_responce(char* buf, const size_t buf_size, struct ghttp__responce* responce)
{
	char temp[64] = {0};

	memset(buf, '\0', buf_size);

	strcat(buf, "HTTP/1.1 ");
	
	sprintf(buf, "%i", responce->code);
	strcat(buf, temp);
	strcat(buf, " ");

	strcat(buf, responce->msg);
	strcat(buf, BRBN);

	ghttp__create_responce_headers(buf, &responce->responce_headers);
}

void ghttp__create_request(char* buf, const size_t buf_size, struct ghttp__request* request);

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
static void ghttp__create_general_headers(char* buf, struct ghttp__general_headers* headers);

static void ghttp__create_responce_headers(char* buf, struct ghttp__responce_headers* headers)
{
	set_if_null(headers->age.name                , age_name);
	set_if_null(headers->allow.name              , allow_name);
	set_if_null(headers->location.name           , location_name);
	set_if_null(headers->proxy_authenticate.name , proxy_authenticate_name);
	set_if_null(headers->range.name              , range_name);
	set_if_null(headers->server.name             , server_name);
	set_if_null(headers->www_authenticate.name   , www_authenticate_name);

	ghttp__add_header(buf, &headers->age);
	ghttp__add_header(buf, &headers->allow);
	ghttp__add_header(buf, &headers->location);
	ghttp__add_header(buf, &headers->proxy_authenticate);
	ghttp__add_header(buf, &headers->range);
	ghttp__add_header(buf, &headers->server);
	ghttp__add_header(buf, &headers->www_authenticate);

	ghttp__create_general_headers(buf, &headers->general_headers);
}

static void ghttp__add_header(char* buf, const struct ghttp__header* header)
{
	if (header->value == NULL) return;

	strcat(buf, header->name);
	strcat(buf, ": ");
	strcat(buf, header->value);
	strcat(buf, BRBN);
}

static const char* accept_charset_name    = "Accept-Charset";
static const char* accept_name            = "Accept";
static const char* accept_ch_name         = "Accept-CH";
static const char* accept_encoding_name   = "Accept-Encoding";
static const char* accept_language_name   = "Accept-Language";
static const char* accept_ranges_name     = "Accept-Ranges";
static const char* content_language_name  = "Content-Language";
static const char* content_encoding_name  = "Content-Encoding";
static const char* content_lenght_name    = "Content-Length";
static const char* content_location_name  = "Content-Location";
static const char* content_range_name     = "Content-Range";
static const char* content_type_name      = "Content-Type";
static const char* connection_name        = "Connection";
static const char* date_name              = "Date";
static const char* pragma_name            = "Pragma";
static const char* transfer_encoding_name = "Transfer-Encoding";
static const char* user_agent_name        = "User-Agent";
static const char* vary_name              = "Vary";
static const char* via_name               = "Via";
static const char* warning_name           = "Warning";

static void ghttp__create_general_headers(char* buf, struct ghttp__general_headers* headers)
{
	set_if_null(headers->accept_charset.name    , accept_charset_name);
	set_if_null(headers->accept.name            , accept_name);
	set_if_null(headers->accept_ch.name         , accept_ch_name);
	set_if_null(headers->accept_encoding.name   , accept_encoding_name);
	set_if_null(headers->accept_language.name   , accept_language_name);
	set_if_null(headers->accept_ranges.name     , accept_ranges_name);
	set_if_null(headers->content_language.name  , content_language_name);
	set_if_null(headers->content_encoding.name  , content_encoding_name);
	set_if_null(headers->content_lenght.name    , content_lenght_name);
	set_if_null(headers->content_location.name  , content_location_name);
	set_if_null(headers->content_range.name     , content_range_name);
	set_if_null(headers->content_type.name      , content_type_name);
	set_if_null(headers->connection.name        , connection_name);
	set_if_null(headers->date.name              , date_name);
	set_if_null(headers->pragma.name            , pragma_name);
	set_if_null(headers->transfer_encoding.name , transfer_encoding_name);
	set_if_null(headers->user_agent.name        , user_agent_name);
	set_if_null(headers->vary.name              , vary_name);
	set_if_null(headers->via.name               , via_name);
	set_if_null(headers->warning.name           , warning_name);

	ghttp__add_header(buf, &headers->warning);
	ghttp__add_header(buf, &headers->accept);
	ghttp__add_header(buf, &headers->accept_ch);
	ghttp__add_header(buf, &headers->accept_charset);
	ghttp__add_header(buf, &headers->accept_encoding);
	ghttp__add_header(buf, &headers->accept_language);
	ghttp__add_header(buf, &headers->accept_ranges);
	ghttp__add_header(buf, &headers->content_language);
	ghttp__add_header(buf, &headers->content_encoding);
	ghttp__add_header(buf, &headers->content_lenght);
	ghttp__add_header(buf, &headers->content_range);
	ghttp__add_header(buf, &headers->content_location);
	ghttp__add_header(buf, &headers->content_type);
	ghttp__add_header(buf, &headers->connection);
	ghttp__add_header(buf, &headers->date);
	ghttp__add_header(buf, &headers->pragma);
	ghttp__add_header(buf, &headers->transfer_encoding);
	ghttp__add_header(buf, &headers->user_agent);
	ghttp__add_header(buf, &headers->vary);
	ghttp__add_header(buf, &headers->via);
}
