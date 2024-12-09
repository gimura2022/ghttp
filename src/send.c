#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <ghttp/messanges.h>
#include <ghttp/send.h>
#include <ghttp/gen.h>
#include <ghttp/ghttp.h>
#include <ghttp/parse.h>
#include <ghttp/utils.h>

#include <glog.h>

#include <gstd/strref.h>

#define RECV_BUFFER_SIZE 64 * 1024

static bool send_data(const void* data, size_t data_size, void** out_data, size_t* out_data_size, int fd);

struct ghttp__responce ghttp__send_request(const struct ghttp__request* request, const char* ip, int port,
		void** to_free)
{
	*to_free = NULL;

	struct ghttp__responce responce = {0};

	int fd;
	struct sockaddr_in serv_addr = {0};
	struct hostent* server = NULL;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		glog__error(ghttp__logger, "socket error");
		return responce;
	}

	server = gethostbyname((char*) ip);
	if (server == NULL) {
		glog__error(ghttp__logger, "get host name error");
		close(fd);
		return responce;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port   = htons(port);
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

	if (connect(fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		glog__error(ghttp__logger, "connect error");
		close(fd);
		return responce;
	}

	void *out, *in;
	size_t out_size, in_size;
	ghttp__gen_request(request, (char**) &out, &out_size);

	char* line;
	ghttp__get_first_line(out, &line);
	glog__infof(ghttp__logger, "to > %s", line);
	ghttp__memmanager->deallocator(line);

	if (!send_data(out, out_size, &in, &in_size, fd)) {
		glog__error(ghttp__logger, "sending/reciving error");
		goto done;
	}

	*to_free = in;

	ghttp__get_first_line(in, &line);
	glog__infof(ghttp__logger, "from < %s", line);
	ghttp__memmanager->deallocator(line);

	if (!ghttp__parse_responce(&(struct gstd__strref) { .start = in, .end = in + in_size, .next = NULL },
			&responce)) {
		glog__error(ghttp__logger, "request syntax error");
		goto done;
	}

done:
	ghttp__memmanager->deallocator(out);
	return responce;
}

struct ghttp__request ghttp__send_responce(const struct ghttp__responce* responce, int fd, void** to_free)
{
	*to_free = NULL;

	struct ghttp__request request = {0};

	void *out, *in;
	size_t out_size, in_size;
	ghttp__gen_responce(responce, (char**) &out, &out_size);

	char* line;
	ghttp__get_first_line(out, &line);
	glog__infof(ghttp__logger, "to > %s", out);
	ghttp__memmanager->deallocator(line);

	if (!send_data(out, out_size, &in, &in_size, fd)) {
		glog__error(ghttp__logger, "sending/reciving error");
		goto done;
	}

	*to_free = in;

	ghttp__get_first_line(in, &line);
	glog__infof(ghttp__logger, "from < %s", in);
	ghttp__memmanager->deallocator(line);

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
	send(fd, data, data_size, 0);

	*out_data = ghttp__memmanager->allocator(RECV_BUFFER_SIZE);
	if ((*out_data_size = recv(fd, *out_data, RECV_BUFFER_SIZE, 0)) <= 0)
		return false;

	return true;
}
