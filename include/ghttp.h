#ifndef _ghttp_h
#define _ghttp_h

#include <glog.h>
#include <gstd/allocators.h>

extern struct gstd__memmanager* ghttp__memmanager;
extern struct glog__logger* ghttp__logger;

void ghttp__init(struct gstd__memmanager* memmanager, struct glog__logger* logger);

#endif
