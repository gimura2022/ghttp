#include <stddef.h>
#include <string.h>

#include <ghttp/utils.h>
#include <ghttp/messanges.h>
#include <ghttp/ghttp.h>

#include <gstd/strref.h>

void ghttp__get_first_line(const char* str, char** out)
{
	const char* p = strstr(str, GHTTP__BRBN);
	struct gstd__strref strref = {
		.start = str,
		.end   = p,
		.next  = NULL,
	};

	if (p == NULL) 
		strref.end = str + strlen(str) - 1;

	*out = ghttp__memmanager->allocator(gstd__strref_len(&strref) + 1);
	gstd__str_from_strref(*out, &strref);

	(*out)[gstd__strref_len(&strref)] = '\0';
}
