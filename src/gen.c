#include <stdio.h>
#include <string.h>

#include <gstd/strref.h>

#include <ghttp/gen.h>
#include <ghttp/messanges.h>
#include <ghttp/ghttp.h>

#define MAX_META_SIZE 512
#define MAX_METHOD_SIZE 64
#define MAX_TO_FREE_MAX 64

static const char* semicolon_sep_str = ": ";
static const char* brbn_sep_str      = GHTTP__BRBN;

static struct gstd__strref semicolon_sep;
static struct gstd__strref brbn_sep;

void ghttp__gen_init(void)
{
	semicolon_sep = gstd__strref_from_str(semicolon_sep_str);
	brbn_sep      = gstd__strref_from_str(brbn_sep_str);
}

static void add_brbn_sep(struct gstd__strref* str, void** to_free, size_t* to_free_size);
static void add_semicolon_sep(struct gstd__strref* str, void** to_free, size_t* to_free_size);

static void gen_responce_headers(struct ghttp__responce_headers* headers,
		struct gstd__strref* str, void** to_free, size_t* to_free_size);
static void gen_request_headers(struct ghttp__request_headers* headers,
		struct gstd__strref* str, void** to_free, size_t* to_free_size);
static void gen_general_headers(struct ghttp__general_headers* headers,
		struct gstd__strref* str, void** to_free, size_t* to_free_size);

static void gen_data(const void* data, size_t data_size, struct gstd__strref* str,
		struct gstd__strref* data_str);

static const char* get_responce_message_by_code(int code);

void ghttp__gen_responce(const struct ghttp__responce* responce, char** buf, size_t* out_size)
{
	struct gstd__strref str, data_str;

	void* to_free[MAX_TO_FREE_MAX] = {NULL};
	size_t to_free_size = 0;

	char meta[MAX_META_SIZE];
	sprintf(meta, "HTTP/1.1 %i %s" GHTTP__BRBN, responce->code,
			get_responce_message_by_code(responce->code));

	str = gstd__strref_from_str(meta);

	gen_responce_headers((struct ghttp__responce_headers*) &responce->headers, &str, to_free,
			&to_free_size);
	gen_general_headers((struct ghttp__general_headers*) &responce->headers.general, &str, to_free,
			&to_free_size);

	add_brbn_sep(&str, to_free, &to_free_size);

	if (responce->data != NULL) {
		gen_data(responce->data, responce->data_size, &str, &data_str);
	}

	*out_size = gstd__strref_len(&str);
	*buf = ghttp__memmanager->allocator(*out_size);
	gstd__str_from_strref(*buf, &str);

	for (int i = 0; i < to_free_size; i++)
		ghttp__memmanager->deallocator(to_free[i]);
}

static const char* get_method(int type);

void ghttp__gen_request(const struct ghttp__request* request, char** buf, size_t* out_size)
{
	struct gstd__strref str, data_str;

	void* to_free[MAX_TO_FREE_MAX] = {NULL};
	size_t to_free_size = 0;

	char meta[MAX_META_SIZE];
	char method[MAX_METHOD_SIZE] = {0};
	gstd__str_from_strref(method, &request->get.url);
	sprintf(meta, "%s %s" GHTTP__BRBN, get_method(request->type), method);

	str = gstd__strref_from_str(meta);

	gen_request_headers((struct ghttp__request_headers*) &request->headers, &str, to_free,
			&to_free_size);
	gen_general_headers((struct ghttp__general_headers*) &request->headers.general, &str, to_free,
			&to_free_size);

	add_brbn_sep(&str, to_free, &to_free_size);

	if (request->data != NULL) {
		gen_data(request->data, request->data_size, &str, &data_str);
	}

	*out_size = gstd__strref_len(&str);
	*buf = ghttp__memmanager->allocator(*out_size);
	gstd__str_from_strref(*buf, &str);

	for (int i = 0; i < to_free_size; i++)
		ghttp__memmanager->deallocator(to_free[i]);
}

static void gen_header(struct ghttp__header* header, struct gstd__strref* str, void** to_free,
		size_t* to_free_size);

#define add_header(x, y) headers->x.name = (struct gstd__strref) { .start = y, \
	.end = (char*) ((size_t) y + strlen(y)), .next = NULL }; gen_header(&headers->x, str, to_free, \
			to_free_size);
static void gen_responce_headers(struct ghttp__responce_headers* headers,
		struct gstd__strref* str, void** to_free, size_t* to_free_size)
{
	ghttp__process_responce_headers
}

static void gen_request_headers(struct ghttp__request_headers* headers,
		struct gstd__strref* str, void** to_free, size_t* to_free_size)
{
	ghttp__process_request_headers
}

static void gen_general_headers(struct ghttp__general_headers* headers,
		struct gstd__strref* str, void** to_free, size_t* to_free_size)
{
	ghttp__process_general_headers
}
#undef add_header

static void gen_header(struct ghttp__header* header, struct gstd__strref* str, void** to_free,
		size_t* to_free_size)
{
	if (!header->has)
		return;

	gstd__strref_cat(str, &header->name);
	add_semicolon_sep(str, to_free, to_free_size);
	gstd__strref_cat(str, &header->value);
	add_brbn_sep(str, to_free, to_free_size);
}

static void gen_data(const void* data, size_t data_size, struct gstd__strref* str,
		struct gstd__strref* data_str)
{
	*data_str = (struct gstd__strref) { .start = data, .end = data + data_size, .next = NULL };
	gstd__strref_cat(str, data_str);
}

static const char* status_100 = "Continue";
static const char* status_101 = "Switching Protocols";
static const char* status_102 = "Processing";
static const char* status_103 = "Early Hints";

static const char* status_200 = "OK";
static const char* status_201 = "Created";
static const char* status_202 = "Accepted";
static const char* status_203 = "Non-Authoritative Information";
static const char* status_204 = "No Content";
static const char* status_205 = "Reset Content";
static const char* status_206 = "Partial Content";
static const char* status_207 = "Multi-Status";
static const char* status_208 = "Already Reported";
static const char* status_226 = "IM Used";

static const char* status_300 = "Multiple Choises";
static const char* status_301 = "Moved Permanently";
static const char* status_302 = "Found";
static const char* status_303 = "See Other";
static const char* status_304 = "Not Modified";
static const char* status_305 = "Use Proxy";
static const char* status_307 = "Temporary Redirect";
static const char* status_308 = "Permanent Redirect";

static const char* status_400 = "Bad Request";
static const char* status_401 = "Unauthorized";
static const char* status_402 = "Payment Required";
static const char* status_403 = "Forbidden";
static const char* status_404 = "Not Found";
static const char* status_405 = "Method Not Allowed";
static const char* status_406 = "Not Acceptable";
static const char* status_407 = "Proxy Authentication Required";
static const char* status_408 = "Request Timeout";
static const char* status_409 = "Conflict";
static const char* status_410 = "Gone";
static const char* status_411 = "Length Required";
static const char* status_412 = "Precondition Failed";
static const char* status_413 = "Content Too Large";
static const char* status_414 = "URI Too Long";
static const char* status_415 = "Unsupported Media Type";
static const char* status_416 = "Range Not Satisfiable";
static const char* status_417 = "Expectation Failed";
static const char* status_418 = "I'm a teapot";
static const char* status_421 = "Misdirected Request";
static const char* status_422 = "Unprocessable Content";
static const char* status_423 = "Locked";
static const char* status_424 = "Failed Dependency";
static const char* status_425 = "Too Early";
static const char* status_426 = "Upgrade Required";
static const char* status_428 = "Precondition Required";
static const char* status_429 = "Too Many Requests";
static const char* status_431 = "Request Header Fields Too Large";
static const char* status_451 = "Unavailable For Legal Reasons";

static const char* status_500 = "Internal Server Error";
static const char* status_501 = "Not Implemented";
static const char* status_502 = "Bad Gateway";
static const char* status_503 = "Service Unavailable";
static const char* status_504 = "Gateway Timeout";
static const char* status_505 = "HTTP Version Not Supported";
static const char* status_506 = "Variant Also Negotiates";
static const char* status_507 = "Insufficient Storage";
static const char* status_508 = "Loop Detected";
static const char* status_510 = "Not Extended";
static const char* status_511 = "Network Authentication Required";

static const char* status_undefined = "Undefined";

static const char* get_responce_message_by_code(int code)
{
#	define case(x, y) if (code == x) return y;

	case(100, status_100);
	case(101, status_101);
	case(102, status_102);
	case(103, status_103);

	case(200, status_200);
	case(201, status_201);
	case(202, status_202);
	case(203, status_203);
	case(204, status_204);
	case(205, status_205);
	case(206, status_206);
	case(207, status_207);
	case(208, status_208);
	case(226, status_226);

	case(300, status_300);
	case(301, status_301);
	case(302, status_302);
	case(303, status_303);
	case(304, status_304);
	case(305, status_305);
	case(307, status_307);
	case(308, status_308);

	case(400, status_400);
	case(401, status_401);
	case(402, status_402);
	case(403, status_403);
	case(404, status_404);
	case(405, status_405);
	case(406, status_406);
	case(407, status_407);
	case(408, status_408);
	case(409, status_409);
	case(410, status_410);
	case(411, status_411);
	case(412, status_412);
	case(413, status_413);
	case(414, status_414);
	case(415, status_415);
	case(416, status_416);
	case(417, status_417);
	case(418, status_418);
	case(421, status_421);
	case(422, status_422);
	case(423, status_423);
	case(424, status_424);
	case(425, status_425);
	case(426, status_426);
	case(428, status_428);
	case(429, status_429);
	case(431, status_431);
	case(451, status_451);

	case(500, status_500);
	case(501, status_501);
	case(502, status_502);
	case(503, status_503);
	case(504, status_504);
	case(505, status_505);
	case(506, status_506);
	case(507, status_507);
	case(508, status_508);
	case(510, status_510);
	case(511, status_511);

#	undef case

	return status_undefined;
}

static const char* method_get       = "GET";
static const char* method_undefined = "UNDEFINED";

static const char* get_method(int type)
{
#	define case(x, y) if (type == x) return y;

	case(GHTTP__REQUEST_GET, method_get);

#	undef case

	return method_undefined;
}

static void add_strref_copy(struct gstd__strref* str, const struct gstd__strref* src, void** to_free,
		size_t* to_free_size);

static void add_brbn_sep(struct gstd__strref* str, void** to_free, size_t* to_free_size)
{
	add_strref_copy(str, &brbn_sep, to_free, to_free_size);
}

static void add_semicolon_sep(struct gstd__strref* str, void** to_free, size_t* to_free_size)
{
	add_strref_copy(str, &semicolon_sep, to_free, to_free_size);
}

static void add_strref_copy(struct gstd__strref* str, const struct gstd__strref* src, void** to_free,
		size_t* to_free_size)
{
	to_free[*to_free_size] = ghttp__memmanager->allocator(sizeof(struct gstd__strref));
	*((struct gstd__strref*) to_free[*to_free_size]) = *src;
	gstd__strref_cat(str, to_free[*to_free_size]);
	(*to_free_size)++;
}
