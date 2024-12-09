#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <glog.h>

#include <gstd/strref.h>

#include <ghttp/ghttp.h>
#include <ghttp/server.h>
#include <ghttp/messanges.h>
#include <ghttp/parse.h>
#include <ghttp/gen.h>
#include <ghttp/utils.h>

#define RECV_BUFFER_SIZE 64 * 1024
#define MAX_NUMBER 128

struct reciver_args {
	int fd;

	struct ghttp__responder* responders;
	size_t responder_count;

	struct ghttp__responder* not_found;
};

static void* reciver(struct reciver_args* args);

void ghttp__start_server(struct ghttp__responder* responders, size_t responders_count,
		struct ghttp__responder* not_found, int port)
{
	glog__debug(ghttp__logger, "starting server");

	int server_fd;
	struct sockaddr_in address;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		glog__die(ghttp__logger, "socket faled");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0)
		glog__die(ghttp__logger, "bind faled");

	if (listen(server_fd, 3) < 0)
		glog__die(ghttp__logger, "listen faled");

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		struct reciver_args* recv_args = ghttp__memmanager->allocator(sizeof(struct reciver_args));
		
		if ((recv_args->fd = accept(server_fd, (struct sockaddr*) &client_addr,
						&client_addr_len)) < 0) continue; 

		recv_args->responders      = responders;
		recv_args->responder_count = responders_count;
		recv_args->not_found       = not_found;

		pthread_t thread_id;
		pthread_create(&thread_id, NULL, (void *(*)(void*)) reciver, (void*) recv_args);
		pthread_detach(thread_id);
	}
}

static struct ghttp__simple_request create_simple_request(struct ghttp__request* request);
static struct ghttp__responce create_responce_from_simple(const struct ghttp__simple_responce* responce);

static void* reciver(struct reciver_args* args)
{
	void* buf = ghttp__memmanager->allocator(RECV_BUFFER_SIZE);

	size_t recv_size;
	if ((recv_size = recv(args->fd, buf, RECV_BUFFER_SIZE, 0) < 0)) {
		glog__error(ghttp__logger, "recive error");
		goto done;		
	}

	char* line;
	ghttp__get_first_line(buf, &line);
	glog__infof(ghttp__logger, "from < %s", line);
	ghttp__memmanager->deallocator(line);

	struct ghttp__request request = {0};
	struct gstd__strref buf_strref = (struct gstd__strref) { .start = buf, .end = buf + recv_size,
		.next = NULL};
	if (!ghttp__parse_request(&buf_strref, &request)) {
		glog__error(ghttp__logger, "parsing error");
		goto done;
	}

	const struct ghttp__responder* match_responder = args->not_found;

	for (int i = 0; i < args->responder_count; i++) {
		const struct ghttp__responder* responder = &args->responders[i];

		if (responder->checker_func != NULL) {
			char* url = ghttp__memmanager->allocator(gstd__strref_len(&request.get.url));

			if (responder->checker_func(url)) {
				match_responder = responder;
				break;
			}

			ghttp__memmanager->deallocator(url);
		}

		struct gstd__strref req_url = gstd__strref_from_str(responder->url);
		if (gstd__strref_cmp(&req_url, &request.get.url) == 0) {
			match_responder = responder;
			break;
		}
	}

	struct ghttp__simple_request simple_request = create_simple_request(&request);
	struct ghttp__simple_responce simple_responce = {0};

	match_responder->process_func(&simple_request, &simple_responce); 

	struct ghttp__responce responce = create_responce_from_simple(&simple_responce);

	char* out;
	size_t out_size;
	ghttp__gen_responce(&responce, &out, &out_size);

	ghttp__get_first_line(out, &line);
	glog__infof(ghttp__logger, "to > %s", line);
	ghttp__memmanager->deallocator(line);

	send(args->fd, out, out_size, 0);

	match_responder->destructor_func(&simple_responce);
	ghttp__memmanager->deallocator(out);
	if (responce.data != NULL)
		ghttp__memmanager->deallocator((void*) responce.headers.general.content_length.value.start);

done:
	ghttp__memmanager->deallocator(buf);
	close(args->fd);

	return NULL;
}

static struct ghttp__simple_request create_simple_request(struct ghttp__request* request)
{
	struct ghttp__simple_request simple_request = {0};
	simple_request.data      = request->data;
	simple_request.data_size = request->data_size;
	simple_request.url       = request->get.url;
	simple_request.real      = request;

	return simple_request;
}

static struct ghttp__responce create_responce_from_simple(const struct ghttp__simple_responce* simple)
{
	struct ghttp__responce responce = {0};
	responce.code = simple->code;
	responce.data = simple->data;
	responce.data_size = simple->data_size;
	
	if (responce.data != NULL) {
		char* num = ghttp__memmanager->allocator(MAX_NUMBER);	
		sprintf(num, "%zu", simple->data_size);

		responce.headers.general.content_length.has = true;
		responce.headers.general.content_length.value = gstd__strref_from_str(num);

		responce.headers.general.content_type.has = true;
		responce.headers.general.content_type.value = gstd__strref_from_str(simple->content_type);
	}

	return responce;
}
