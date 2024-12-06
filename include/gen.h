#ifndef _gen_h
#define _gen_h

#include <ghttp/messanges.h>

void ghttp__gen_responce(const struct ghttp__responce* responce, char** buf);
void ghttp__gen_request(const struct ghttp__request* request, char** buf);

#endif
