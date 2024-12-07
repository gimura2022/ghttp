#ifndef _send_h
#define _send_h

#include <stdbool.h>

#include <ghttp/messanges.h>

struct ghttp__responce ghttp__send_request(const struct ghttp__request* request, const char* ip, int port,
		void** to_free);
struct ghttp__request ghttp__send_responce(const struct ghttp__responce* responce, int fd, void** to_free);

#endif
