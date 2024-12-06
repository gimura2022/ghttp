#ifndef _parse_h
#define _parse_h

#include <stdbool.h>

#include <gstd/strref.h>

#include <ghttp/messanges.h>

bool ghttp__parse_request(const struct gstd__strref str, struct ghttp__request* request);
bool ghttp__parse_responce(const struct gstd__strref str, struct ghttp__responce* responce);

#endif
