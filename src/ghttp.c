#include <ghttp/ghttp.h>

struct gstd__memmanager* ghttp__memmanager = NULL;
struct glog__logger* ghttp__logger         = NULL;

void ghttp__init(struct gstd__memmanager* memmanager, struct glog__logger* logger)
{
	ghttp__memmanager = memmanager;
	ghttp__logger     = logger;
}
