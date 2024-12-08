#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <gstd/strref.h>

#include <ghttp/ghttp.h>
#include <ghttp/parse.h>
#include <ghttp/messanges.h>

#define MAX_NUMBER 128

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
	continue_or_return(parse_data(str, data_start, (const void**) &responce->data,
				&responce->data_size));

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

static bool parse_responce_meta(struct gstd__strref* str, struct ghttp__responce* responce);
static bool parse_responce_body(struct gstd__strref* str, struct ghttp__responce_headers* headers);

static int responce_enumerator(struct gstd__strref* str, struct ghttp__responce* responce, size_t line)
{
	if (line == 0) continue_or_return(parse_responce_meta(str, responce));
	else if (str->start == str->end) return 2;
	else continue_or_return(parse_responce_body(str, &responce->headers));

	return true;
}

static bool parse_get_request(struct gstd__strref* str, struct ghttp__request* request);

static bool parse_request_meta(struct gstd__strref* str, struct ghttp__request* request)
{
#	define case(x, y) ({ const char* __x = strstr(str->start, x); \
		if (__x && __x < str->end && __x == str->start) y; })
	case("GET ", continue_or_return(parse_get_request(str, request)));
#	undef case

	return true;
}

static bool parse_responce_meta(struct gstd__strref* str, struct ghttp__responce* responce)
{
	if (strstr(str->start, "HTTP/1.1") != str->start && strstr(str->start, "HTTP/1.0") != str->start)
		return false;

	char number[MAX_NUMBER] = {0};
	const char* start = str->start + strlen("HTTP/1.1 ");
	const char* p = start;
	while (isdigit(*p)) p++;

	memcpy(number, start, p - start);

	responce->code = atoi(number);
	if (responce->code == 0)
		return false;

	return true;
}

static bool parse_get_request(struct gstd__strref* str, struct ghttp__request* request)
{
	request->type = GHTTP__REQUEST_GET;

	const char* p = strstr(str->start, " HTTP/1.");
	if (p > str->end || p == NULL)
		return false;

	request->get.url = (struct gstd__strref) {
		.start = str->start + strlen("GET "),
		.end   = p,
		.next  = NULL,
	};

	return true;
}

static bool get_header(struct gstd__strref* str, struct ghttp__header* header);
static bool parse_general_body(struct gstd__strref* str, struct ghttp__general_headers* headers);

#define add_header(x, y) ({ struct gstd__strref __x = gstd__strref_from_str(y); \
	if (gstd__strref_cmp(&header.name, &__x) == 0) headers->x = header; });
static bool parse_request_body(struct gstd__strref* str, struct ghttp__request_headers* headers)
{
	struct ghttp__header header;
	continue_or_return(get_header(str, &header));

	ghttp__process_request_headers
	
	continue_or_return(parse_general_body(str, &headers->general));

	return true;
}

static bool parse_responce_body(struct gstd__strref* str, struct ghttp__responce_headers* headers)
{
	struct ghttp__header header;
	continue_or_return(get_header(str, &header));

	ghttp__process_responce_headers
	
	continue_or_return(parse_general_body(str, &headers->general));

	return true;
}

static bool parse_general_body(struct gstd__strref* str, struct ghttp__general_headers* headers)
{
	struct ghttp__header header;
	continue_or_return(get_header(str, &header));

	ghttp__process_general_headers

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
			.start = p + 1,
			.end   = str->end,
			.next  = NULL,
		},
	};

	while (*header->value.start == ' ') header->value.start++;

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
