#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>

#include <ghttp/messanges.h>
#include <ghttp/send.h>
#include <ghttp/gen.h>
#include <ghttp/ghttp.h>
#include <ghttp/parse.h>

#include <glog.h>

#include <gstd/strref.h>

#define RECV_BUFFER_SIZE 64 * 1024

static bool send_data(const void* data, size_t data_size, void** out_data, size_t* out_data_size, int fd);

struct ghttp__responce ghttp__send_request(const struct ghttp__request* request, const char* ip)
{
	
}

struct ghttp__request ghttp__send_responce(const struct ghttp__responce* responce, int fd, void** to_free)
{
	struct ghttp__request request = {0};

	void *out, *in;
	size_t out_size, in_size;
	ghttp__gen_responce(responce, (char**) &out, &out_size);

	glog__infof(ghttp__logger, "> %s", out);

	if (!send_data(out, out_size, &in, &in_size, fd)) {
		glog__error(ghttp__logger, "sending/reciving error");
		goto done;
	}

	*to_free = in;

	glog__infof(ghttp__logger, "< %s", in);

	if (!ghttp__parse_request(&(struct gstd__strref) { .start = in, .end = in + in_size, .next = NULL },
			&request)) {
		glog__error(ghttp__logger, "request syntax error");
		goto done;
	}

done:
	ghttp__memmanager->deallocator(out);
	return request;
}

static bool send_data(const void* data, size_t data_size, void** out_data, size_t* out_data_size, int fd)
{
	if (send(fd, data, data_size, 0) != 0)
		return false;

	*out_data = ghttp__memmanager->allocator(RECV_BUFFER_SIZE);
	if ((*out_data_size = recv(fd, *out_data, RECV_BUFFER_SIZE, 0)) <= 0)
		return false;

	return true;
}
