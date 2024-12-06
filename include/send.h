#ifndef _send_h
#define _send_h

#include <stdbool.h>

#include <ghttp/messanges.h>

bool ghttp__send_request(const struct ghttp__request* request, const char* ip);
bool ghttp__send_responce(const struct ghttp__responce* responce, int fd);

#endif
