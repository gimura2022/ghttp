#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <gstd/strref.h>

#include <ghttp/ghttp.h>
#include <ghttp/parse.h>
#include <ghttp/messanges.h>

#define continue_or_return(x) ({ if (!x) return false; })
#define break_or_return(x) ({ if (!x) return false; break; })

typedef int (*line_enumerator_f)(struct gstd__strref*, void*, size_t);

static bool line_enumerator(const char* str, line_enumerator_f enumerator, void* custom_data,
		const char** data_start);
static bool parse_data(const struct gstd__strref* str, const char* data_start, const void** data,
		size_t* data_size);

static int request_enumerator(struct gstd__strref* str, struct ghttp__request* request, size_t line);

bool ghttp__parse_request(const struct gstd__strref* str, struct ghttp__request* request)
{
	const char* data_start;

	continue_or_return(line_enumerator(str->start, (line_enumerator_f) request_enumerator, request,
				&data_start));
	continue_or_return(parse_data(str, data_start, (const void**) &request->data, &request->data_size));

	return true;
}

static int responce_enumerator(struct gstd__strref* str, struct ghttp__responce* responce, size_t line);

bool ghttp__parse_responce(const struct gstd__strref* str, struct ghttp__responce* responce)
{
	const char* data_start;

	continue_or_return(line_enumerator(str->start, (line_enumerator_f) responce_enumerator, responce,
				&data_start));
	continue_or_return(parse_data(str, data_start, (const void**) &responce->data, &responce->data_size));

	return true;
}

static bool line_enumerator(const char* str, line_enumerator_f enumerator, void* custom_data,
		const char** data_start)
{
	const char *s, *e;
	int line;

	for (s = str, e = strstr(str, GHTTP__BRBN), line = 0;
			(e == NULL ? e = str + strlen(str), true : true) && (e != str + strlen(str));
			s = e, s += 2, e = strstr(s, GHTTP__BRBN), line++) {
		struct gstd__strref strref = (struct gstd__strref) {
			.start = s,
			.end   = e,
			.next  = NULL,
		};

		switch (enumerator(&strref, custom_data, line)) {
		case 0: return false;

		case 2:
			*data_start = s + 2;
			return true;

		default: break;
		}
	}

	*data_start = NULL;

	return true;
}

static bool parse_request_meta(struct gstd__strref* str, struct ghttp__request* request);
static bool parse_request_body(struct gstd__strref* str, struct ghttp__request_headers* headers);

static int request_enumerator(struct gstd__strref* str, struct ghttp__request* request, size_t line)
{
	if (line == 0) continue_or_return(parse_request_meta(str, request));
	else if (str->start == str->end) return 2;
	else continue_or_return(parse_request_body(str, &request->headers));

	return true;
}

static bool parse_get_request(struct gstd__strref* str, struct ghttp__request* request);

static bool parse_request_meta(struct gstd__strref* str, struct ghttp__request* request)
{
	continue_or_return(parse_get_request(str, request));

	return true;
}

static bool parse_get_request(struct gstd__strref* str, struct ghttp__request* request)
{
	request->type = GHTTP__REQUEST_GET;
	request->get.url = (struct gstd__strref) {
		.start = str->start + strlen("GET "),
		.end   = str->end,
		.next  = NULL,
	};

	return true;
}

static bool get_header(struct gstd__strref* str, struct ghttp__header* header);

#define add_header(x, y) struct gstd__strref __x = gstd__strref_from_str(y); \
	if (gstd__strref_cmp(&header.name, &__x) == 0) headers->x = header;
static bool parse_request_body(struct gstd__strref* str, struct ghttp__request_headers* headers)
{
	struct ghttp__header header;
	continue_or_return(get_header(str, &header));

	ghttp__process_request_headers

	return true;
}
#undef add_header

static bool get_header(struct gstd__strref* str, struct ghttp__header* header)
{
	const char* p = strstr(str->start, ":");
	if (p > str->end || p == NULL)
		return false;

	*header = (struct ghttp__header) {
		.has = true,
		.name = (struct gstd__strref) {
			.start = str->start,
			.end   = p,
			.next  = NULL,
		},
		.value = (struct gstd__strref) {
			.start = p,
			.end   = str->end,
			.next  = NULL,
		},
	};

	return true;
}

static bool parse_data(const struct gstd__strref* str, const char* data_start, const void** data,
		size_t* data_size)
{
	if (data_start == NULL)
		return true;

	*data      = data_start;
	*data_size = str->end - data_start;

	return true;
}
