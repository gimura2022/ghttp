#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <ghttp_msg.h>
#include <ghttp.h>

#include <glex.h>
#include <glog.h>

#define BRBN "\r\n"
#define GHTTP_MAX_URL 1024 * 2
#define GHTTP_MAX_TYPE 128

enum {
	TOK_SPACE = 0,

	TOK_GET,
	TOK_HEAD,
	TOK_POST,
	TOK_PUT,
	TOK_DELEATE,
	TOK_CONNECT,
	TOK_OPTIONS,
	TOK_TRACE,
	TOK_PATCH,
};

#define text_compare_checker(x, y) static bool x(const char* s) { return !strcmp(y, s); }
#define array_lenght(x) sizeof(x) / sizeof(x[0])

text_compare_checker(get_tok_checker     , "GET")
text_compare_checker(head_tok_checker    , "HEAD")
text_compare_checker(post_tok_checker    , "POST")
text_compare_checker(put_tok_checker     , "PUT")
text_compare_checker(deleate_tok_checker , "DELEATE")
text_compare_checker(connect_tok_checker , "CONNECT")
text_compare_checker(options_tok_checker , "OPTIONS")
text_compare_checker(trace_tok_checker   , "TRACE")
text_compare_checker(patch_tok_checker   , "PATCH")

static struct glog__logger glex_logger = {0};
static struct glex__token_def token_defs[] = {
	(struct glex__token_def) {
		.def_type = GLEX__TOKENDEF_TYPE__IGNORE_SEPARATOR,
		.type     = TOK_SPACE,
		.sym      = ' ',
		
		.reader     = NULL,
		.destructor = NULL,
	},

#	define const_text_tok_def(x, y) (struct glex__token_def) { .def_type = GLEX__TOKENDEF_TYPE__TOKEN, \
		.type = x, .text_chacker = y, .reader = NULL, .destructor = NULL },

	const_text_tok_def(TOK_GET     , get_tok_checker)
	const_text_tok_def(TOK_HEAD    , head_tok_checker)
	const_text_tok_def(TOK_POST    , post_tok_checker)
	const_text_tok_def(TOK_PUT     , put_tok_checker)
	const_text_tok_def(TOK_DELEATE , deleate_tok_checker)
	const_text_tok_def(TOK_CONNECT , connect_tok_checker)
	const_text_tok_def(TOK_OPTIONS , options_tok_checker)
	const_text_tok_def(TOK_TRACE   , trace_tok_checker)
	const_text_tok_def(TOK_PATCH   , patch_tok_checker)

#	undef const_text_tok_def
};

void ghttp__init_msg(void)
{
	glog__logger_from_prefix(&glex_logger, "glex");
	glex_logger.format           = ghttp__logger->format;
	glex_logger.min_log_level    = ghttp__logger->min_log_level;
	glex_logger.out_stream_count = ghttp__logger->out_stream_count;
	glex_logger.out_streams      = ghttp__logger->out_streams;
}

static bool parse_general_headers(const char* line, struct ghttp__general_headers* headers);
static bool parse_request_headers(const char* line, struct ghttp__request_headers* headers);
static bool parse_responce_headers(const char* line, struct ghttp__responce_headers* headers);
static bool parse_content(const void* content, const struct ghttp__header* content_length,
		void** content_out);

bool ghttp__parse_request(struct ghttp__request* request, const char* str)
{
	bool return_value = false;

	char* buf = ghttp__malloc(strlen(str));
	memset(buf, '\0', strlen(str));
	strcpy(buf, str);

	char* save_ptr;
	char* s = strtok_r(buf, BRBN, &save_ptr);

	request->type = ghttp__malloc(GHTTP_MAX_TYPE);
	request->url  = ghttp__malloc(GHTTP_MAX_URL);

	if (sscanf(s, "%s %s HTTP/1.1", request->type, request->url) != 2)
		goto done;

	for (s = strtok_r(NULL, BRBN, &save_ptr); 
			s != NULL && strcmp(s, "") != 0; s = strtok_r(NULL, BRBN, &save_ptr)) {
		if (!parse_general_headers(s, &request->headers.general)) goto done;
		if (!parse_request_headers(s, &request->headers)) goto done;
	}

	if (!parse_content(strtok_r(NULL, "", &save_ptr), &request->headers.general.content_length,
			&request->content)) goto done;

	return_value = true;

done:
	ghttp__free(buf);
	return return_value;
}

bool ghttp__parse_responce(struct ghttp__responce* responce, const char* str)
{
	bool return_value = false;

	char* buf = ghttp__malloc(strlen(str));
	memset(buf, '\0', strlen(str));
	strcpy(buf, str);

	char* save_ptr;
	char* s = strtok_r(buf, BRBN, &save_ptr);

	if (sscanf(s, "HTTP/1.1 %i %s", &responce->responce_code, responce->responce_str) != 2)
		goto done;

	for (s = strtok_r(NULL, BRBN, &save_ptr); 
			strcmp(s, "") != 0 && s != NULL; s = strtok_r(NULL, BRBN, &save_ptr)) {
		if (!parse_general_headers(s, &responce->headers.general)) goto done;
		if (!parse_responce_headers(s, &responce->headers)) goto done;
	}

	if (!parse_content(strtok_r(NULL, "", &save_ptr), &responce->headers.general.content_length,
			&responce->content)) goto done;

done:
	ghttp__free(buf);
	return return_value;
}

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
	add_content(request->content, str, out_size, &request->headers.general.content_length);
}

void ghttp__create_responce(const struct ghttp__responce* responce, char* str, size_t* out_size)
{
	sprintf(str, "HTTP/1.1 %i %s" BRBN, responce->responce_code, responce->responce_str);
	add_general_headers(&responce->headers.general, str);
	add_responce_headers(&responce->headers, str);
	add_content(responce->content, str, out_size, &responce->headers.general.content_length);
}

#define free_if_non_null(x) if (x != NULL) ghttp__free(x);
#define free_header_with_headers(x, header) free_if_non_null(header.x.name); \
		free_if_non_null(header.x.value);

void ghttp__free_request(const struct ghttp__request* request)
{
	free_if_non_null(request->content);
	free_if_non_null(request->type);
	free_if_non_null(request->url);

#	define add_header(x, y) free_header_with_headers(x, request->headers);
	request_headers
#	undef add_header
#	define add_header(x, y) free_header_with_headers(x, request->headers.general);
	general_headers
#	undef add_header
}

void ghttp__free_responce(const struct ghttp__responce* responce)
{
	free_if_non_null(responce->content);
	free_if_non_null(responce->responce_str);
	
#	define add_header(x, y) free_header_with_headers(x, responce->headers);
	responce_headers
#	undef add_header
#	define add_header(x, y) free_header_with_headers(x, responce->headers.general);
	general_headers
#	undef add_header
}

#undef free_if_non_null
#undef free_header_with_headers

static void add_header_to_buf(const char* name, const struct ghttp__header* header, char* str);

static void add_general_headers(const struct ghttp__general_headers* headers, char* str)
{
#	define add_header(x, y) add_header_to_buf(y, &headers->x, str);
	general_headers
#	undef add_header
}

static void add_request_headers(const struct ghttp__request_headers* headers, char* str)
{
#	define add_header(x, y) add_header_to_buf(y, &headers->x, str);
	request_headers
#	undef add_header
}

static void add_responce_headers(const struct ghttp__responce_headers* headers, char* str)
{
#	define add_header(x, y) add_header_to_buf(y, &headers->x, str);
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

static bool parse_header(const char* line, const char* name, struct ghttp__header* header);

static bool parse_general_headers(const char* line, struct ghttp__general_headers* headers)
{
#	define add_header(x, y) if (!parse_header(line, y, &headers->x)) return false;
	general_headers
#	undef add_header

	return true;
}

static bool parse_request_headers(const char* line, struct ghttp__request_headers* headers)
{
#	define add_header(x, y) if (!parse_header(line, y, &headers->x)) return false;
	request_headers
#	undef add_header

	return true;
}

static bool parse_responce_headers(const char* line, struct ghttp__responce_headers* headers)
{
#	define add_header(x, y) if (!parse_header(line, y, &headers->x)) return false;
	responce_headers
#	undef add_header

	return true;
}

static bool parse_header(const char* line, const char* name, struct ghttp__header* header)
{
	bool return_value = false;

	char* buf = ghttp__malloc(strlen(line));
	memset(buf, '\0', strlen(line));
	strcpy(buf, line);

	char* save_ptr;
	const char* key   = strtok_r(buf, ": ", &save_ptr);
	const char* value = strtok_r(NULL, "", &save_ptr);

	if (key == NULL || value == NULL) goto done;
	if (strcmp(key, name) != 0) {
		return_value = true;
		goto done;
	}

	header->name  = ghttp__malloc(strlen(key));
	header->value = ghttp__malloc(strlen(value));
	memset(header->name, '\0', strlen(header->name));
	memset(header->value, '\0', strlen(header->value));
	strcpy(header->name, key);
	strcpy(header->value, value);

	return_value = true;

done:
	ghttp__free(buf);
	return return_value;
}

static bool parse_content(const void* content, const struct ghttp__header* content_length,
		void** content_out)
{
	if (content_length->value != NULL) {
		size_t content_length_int = 0;
		if (sscanf(content_length->value, "%zu", &content_length_int)
				!= 1) return false;

		*content_out = ghttp__malloc(content_length_int);
		memset(*content_out, '\0', content_length_int);
		memcpy(*content_out, content, content_length_int);
	} else {
		*content_out = NULL;
	}

	return true;
}
