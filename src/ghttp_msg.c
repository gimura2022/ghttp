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
#define GHTTP_MAX_HEADER_VALUE 2048

enum {
	TOK_SPACE = 0,
	TOK_BR,
	TOK_BN,
	TOK_SEMICOLON,

	TOK_GET,
	TOK_HEAD,
	TOK_POST,
	TOK_PUT,
	TOK_DELEATE,
	TOK_CONNECT,
	TOK_OPTIONS,
	TOK_TRACE,
	TOK_PATCH,

	TOK_HTTP_VER,

	TOK_ANY_TEXT,
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

text_compare_checker(http_ver_tok_checker , "HTTP/1.1");

static bool any_text_checker(const char* s) { return true; }

static struct glog__logger glex_logger = {0};
static struct glex__token_def token_defs[] = {
	(struct glex__token_def) {
		.def_type = GLEX__TOKENDEF_TYPE__IGNORE_SEPARATOR,
		.type     = TOK_SPACE,
		.sym      = ' ',
		
		.reader     = NULL,
		.destructor = NULL,
	},

	(struct glex__token_def) {
		.def_type = GLEX__TOKENDEF_TYPE__SEPARATOR,
		.type     = TOK_BR,
		.sym      = '\r',
		
		.reader     = NULL,
		.destructor = NULL,
	},
	(struct glex__token_def) {
		.def_type = GLEX__TOKENDEF_TYPE__SEPARATOR,
		.type     = TOK_BN,
		.sym      = '\n',
		
		.reader     = NULL,
		.destructor = NULL,
	},
	(struct glex__token_def) {
		.def_type = GLEX__TOKENDEF_TYPE__SEPARATOR,
		.type     = TOK_SEMICOLON,
		.sym      = ':',
		
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

	const_text_tok_def(TOK_HTTP_VER , http_ver_tok_checker)

	const_text_tok_def(TOK_ANY_TEXT, any_text_checker)

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

static bool get_parse(struct ghttp__request* request, struct glex__lexer* lexer);

static bool general_headers_parse(struct ghttp__general_headers* headers, struct glex__lexer* lexer);
static bool request_headers_parse(struct ghttp__request_headers* headers, struct glex__lexer* lexer);

#define unexepted_token() ({ glog__error(ghttp__logger, "unexepted token"); return false; })
#define except_token(lexer, x) ({ if (glex__get_tok(lexer)->type != x) unexepted_token(); })
#define continue_or_return(x) ({ if (!x) return false; })

bool ghttp__parse_request(struct ghttp__request* request, const char* str)
{
	struct glex__lexer lexer = {0};
	glex__set_token_defs(&lexer, array_lenght(token_defs), token_defs);
	glex__parse_string(&lexer, str);

	struct glex__token* tok = glex__get_tok(&lexer);	
	if (tok == NULL) unexepted_token();

	switch (tok->type) {
#	define continue_break_or_return(x) ({ if (!x) return false; break; })

	case TOK_GET: continue_break_or_return(get_parse(request, &lexer));

#	undef continue_break_or_return
	
	default: unexepted_token();
	}

	continue_or_return(general_headers_parse(&request->headers.general, &lexer));
	continue_or_return(request_headers_parse(&request->headers, &lexer));

	glex__free_lexer(&lexer);

	return true;
}

bool ghttp__parse_responce(struct ghttp__responce* responce, const char* str)
{
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
	free_if_non_null(request->url);

#	define add_header(x, y, z, w) free_header_with_headers(x, request->headers);
	request_headers
#	undef add_header
#	define add_header(x, y, z, w) free_header_with_headers(x, request->headers.general);
	general_headers
#	undef add_header
}

void ghttp__free_responce(const struct ghttp__responce* responce)
{
	free_if_non_null(responce->content);
	free_if_non_null(responce->responce_str);
	
#	define add_header(x, y, z, w) free_header_with_headers(x, responce->headers);
	responce_headers
#	undef add_header
#	define add_header(x, y, z, w) free_header_with_headers(x, responce->headers.general);
	general_headers
#	undef add_header
}

#undef free_if_non_null
#undef free_header_with_headers

static void add_header_to_buf(const char* name, const struct ghttp__header* header, char* str,
		const char* value_pattern, void* data);

static void add_general_headers(const struct ghttp__general_headers* headers, char* str)
{
#	define add_header(x, y, z, w) add_header_to_buf(y, &headers->x, str, w, (void*) &headers->x##_data);
	general_headers
#	undef add_header
}

static void add_request_headers(const struct ghttp__request_headers* headers, char* str)
{
#	define add_header(x, y, z, w) add_header_to_buf(y, &headers->x, str, w, (void*) &headers->x##_data);
	request_headers
#	undef add_header
}

static void add_responce_headers(const struct ghttp__responce_headers* headers, char* str)
{
#	define add_header(x, y, z, w) add_header_to_buf(y, &headers->x, str, w, (void*) &headers->x##_data);
	responce_headers
#	undef add_header
}

static void add_header_to_buf(const char* name, const struct ghttp__header* header, char* str,
		const char* value_pattern, void* data)
{
	if (header->value != NULL) {
add_str:
		strcat(str, name);
		strcat(str, ": ");
		strcat(str, header->value);
		strcat(str, BRBN);

		return;
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

static bool get_parse(struct ghttp__request* request, struct glex__lexer* lexer)
{
	request->type = "GET";

	struct glex__token* tok;

	tok = glex__get_tok(lexer);
	if (tok == NULL || tok->type != TOK_ANY_TEXT)
		unexepted_token();

	request->url = ghttp__malloc(strlen(tok->text));
	strcpy(request->url, tok->text);

	except_token(lexer, TOK_BR);
	except_token(lexer, TOK_BN);

	return true;
}

static bool header_parse(struct ghttp__header* header, char* name, const char* real_name,
	char* value, const char* value_pattern, void* data);

static bool general_headers_parse(struct ghttp__general_headers* headers, struct glex__lexer* lexer)
{
	struct glex__token* tok;

	while (true) {
		tok = glex__get_tok(lexer);

		if (tok->type == TOK_BR) {
			except_token(lexer, TOK_BN);
			except_token(lexer, TOK_BR);
			except_token(lexer, TOK_BN);

			break;
		} else if (tok->type == TOK_ANY_TEXT);
		else unexepted_token();
		
		char* name = ghttp__malloc(strlen(tok->text));
		strcpy(name, tok->text);

		except_token(lexer, TOK_SEMICOLON);

		struct glex__token *data_start, *data_end;
		data_start = lexer->cur_tok;

		while ((tok = glex__get_tok(lexer))->type != TOK_BR) {
			data_end = lexer->cur_tok;
		}
		except_token(lexer, TOK_BN);

		char* value = ghttp__malloc(GHTTP_MAX_HEADER_VALUE);
		memset(value, '\0', GHTTP_MAX_HEADER_VALUE);
		for (struct glex__token* i = data_start; i != data_end; i = i->next) {
			strcat(value, i->text);
		}

#		define add_header(x, y, z, w) if (header_parse(&headers->x, name, y, value, w, \
			&headers->x##_data)) break;
		general_headers
#		undef add_header

		ghttp__free(name);
		ghttp__free(value);
	}

	return true;
}

static bool request_headers_parse(struct ghttp__request_headers* headers, struct glex__lexer* lexer)
{
	struct glex__token* tok;

	while (true) {
		tok = glex__get_tok(lexer);

		if (tok->type == TOK_BR) {
			except_token(lexer, TOK_BN);
			except_token(lexer, TOK_BR);
			except_token(lexer, TOK_BN);

			break;
		} else if (tok->type == TOK_ANY_TEXT);
		else unexepted_token();
		
		char* name = ghttp__malloc(strlen(tok->text));
		strcpy(name, tok->text);

		except_token(lexer, TOK_SEMICOLON);

		struct glex__token *data_start, *data_end;
		data_start = lexer->cur_tok;

		while ((tok = glex__get_tok(lexer))->type != TOK_BR) {
			data_end = lexer->cur_tok;
		}
		except_token(lexer, TOK_BN);

		char* value = ghttp__malloc(GHTTP_MAX_HEADER_VALUE);
		memset(value, '\0', GHTTP_MAX_HEADER_VALUE);
		for (struct glex__token* i = data_start; i != data_end; i = i->next) {
			strcat(value, i->text);
		}

#		define add_header(x, y, z, w) if (header_parse(&headers->x, name, y, value, w, \
			&headers->x##_data)) break;
		request_headers
#		undef add_header

		ghttp__free(name);
		ghttp__free(value);
	}

	return true;
}

static bool header_parse(struct ghttp__header* header, char* name, const char* real_name,
	char* value, const char* value_pattern, void* data)
{
	if (strcmp(name, real_name) != 0)
		return false;

	header->name  = name;
	header->value = value;

	if (value_pattern == NULL)
		return true;

	if (sscanf(value, value_pattern, data) != 1)
		return false;

	return true;
}
