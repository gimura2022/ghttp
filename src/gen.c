#include <stdio.h>

#include <gstd/strref.h>

#include <ghttp/gen.h>
#include <ghttp/messanges.h>
#include <ghttp/ghttp.h>

#define MAX_META_SIZE 512

static const char* semicolon_sep_str = ": ";
static const char* brbn_sep_str      = GHTTP__BRBN;

static struct gstd__strref semicolon_sep;
static struct gstd__strref brbn_sep;

void ghttp__gen_init(void)
{
	semicolon_sep = gstd__strref_from_str(semicolon_sep_str);
	brbn_sep      = gstd__strref_from_str(brbn_sep_str);
}

static void gen_responce_headers(struct ghttp__responce_headers* headers,
		struct gstd__strref* str);
static void gen_request_headers(struct ghttp__request_headers* headers,
		struct gstd__strref* str);
static void gen_general_headers(struct ghttp__general_headers* headers,
		struct gstd__strref* str);

static void gen_data(const void* data, size_t data_size, struct gstd__strref* str);

static const char* get_responce_message_by_code(int code);

void ghttp__gen_responce(const struct ghttp__responce* responce, char** buf, size_t* out_size)
{
	struct gstd__strref str;

	char meta[MAX_META_SIZE];
	sprintf(meta, "HTTP/1.1 %i %s" GHTTP__BRBN, responce->code,
			get_responce_message_by_code(responce->code));

	str = gstd__strref_from_str(meta);

	gen_responce_headers((struct ghttp__responce_headers*) &responce->headers, &str);
	gen_general_headers((struct ghttp__general_headers*) &responce->headers.general, &str);

	if (responce->data != NULL && responce->headers.general.content_length.has) {
		gen_data(responce->data, responce->data_size, &str);
	}

	*out_size = gstd__strref_len(&str);
	*buf = ghttp__memmanager->allocator(*out_size);
	gstd__str_from_strref(*buf, &str);
}

void ghttp__gen_request(const struct ghttp__request* request, char** buf, size_t* out_size);

static void gen_header(struct ghttp__header* header, struct gstd__strref* str);

#define add_header(x, y) gen_header(&headers->x, str);
static void gen_responce_headers(struct ghttp__responce_headers* headers,
		struct gstd__strref* str)
{
	ghttp__process_responce_headers
}

static void gen_request_headers(struct ghttp__request_headers* headers,
		struct gstd__strref* str)
{
	ghttp__process_request_headers
}

static void gen_general_headers(struct ghttp__general_headers* headers,
		struct gstd__strref* str)
{
	ghttp__process_general_headers
}
#undef add_header

static void gen_header(struct ghttp__header* header, struct gstd__strref* str)
{
	if (!header->has)
		return;

	gstd__strref_cat(str, &header->name);
	gstd__strref_cat(str, &semicolon_sep);
	gstd__strref_cat(str, &header->value);
	gstd__strref_cat(str, &brbn_sep);
}

static void gen_data(const void* data, size_t data_size, struct gstd__strref* str)
{
	gstd__strref_cat(str, &(struct gstd__strref) { .start = data, .end = data + data_size,
			.next = NULL });
}

static const char* status_200       = "OK";
static const char* status_404       = "Not Found";
static const char* status_undefined = "Undefined";

static const char* get_responce_message_by_code(int code)
{
#	define case(x, y) if (code == x) return y;

	case(200, status_200);
	case(404, status_404);

#	undef case

	return status_undefined;
}
