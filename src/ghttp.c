#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <regex.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <threads.h>

#include <ghttp.h>
#include <ghttp_msg.h>

#define GHTTP_BUFFER_SIZE 1024 * 64

struct ghttp__server_data ghttp__get_default_server_data(void)
{
	return (struct ghttp__server_data) {
		.port = 8080
	};
}

struct ghttp__reciver_args {
	int client_fd;
	struct ghttp__server_context* server_context;
	struct ghttp__path_responder* responders;
	size_t responder_count;
	ghttp__respoder_t not_found;
};

static void die(const char* msg)
{
	printf("Error: %s\n", msg);
	exit(1);
}

static void warn(const char* msg)
{
	printf("Warn: %s\n", msg);
}

static void dief(const char* fmt, ...)
{
	va_list	args;
	va_start(args, fmt);

	char buf[GHTTP_BUFFER_SIZE] = {0};
	vsprintf(buf, fmt, args);

	va_end(args);

	die(buf);
}

static void warnf(const char* fmt, ...)
{
	va_list	args;
	va_start(args, fmt);

	char buf[GHTTP_BUFFER_SIZE] = {0};
	vsprintf(buf, fmt, args);

	va_end(args);

	warn(buf);
}

ghttp__malloc_t ghttp__malloc;
ghttp__free_t ghttp__free;

void ghttp__init(ghttp__malloc_t allocator, ghttp__free_t deallocator)
{
	ghttp__malloc = allocator;
	ghttp__free   = deallocator;
}

static void* ghttp__reciver(struct ghttp__reciver_args* args);

void ghttp__start_server(struct ghttp__server_data server_data, struct ghttp__path_responder* responders,
		size_t responder_count, ghttp__respoder_t not_found_responder)
{
	int server_fd;
	struct sockaddr_in address;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("socket failed");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(server_data.port);

	if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0)
		die("bind failed");
	if (listen(server_fd, 3) < 0)
		die("listen failed");

	struct ghttp__server_context context = {
		.runned = true
	};

	while (context.runned) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		struct ghttp__reciver_args* http_rec_args
			= ghttp__malloc(sizeof(struct ghttp__reciver_args));
		
		if ((http_rec_args->client_fd = accept(server_fd, (struct sockaddr*) &client_addr,
						&client_addr_len)) < 0) continue; 
		http_rec_args->server_context  = &context;
		http_rec_args->responders      = responders;
		http_rec_args->responder_count = responder_count;
		http_rec_args->not_found       = not_found_responder;

		pthread_t thread_id;
		pthread_create(&thread_id, NULL, (void *(*)(void*)) ghttp__reciver, (void*) http_rec_args);	
		pthread_detach(thread_id);
	}
}

static void* ghttp__reciver(struct ghttp__reciver_args* args)
{
	char* buf = ghttp__malloc(GHTTP_BUFFER_SIZE);
	ssize_t recv_size = recv(args->client_fd, buf, GHTTP_BUFFER_SIZE, 0);

	if (recv_size <= 0) goto done;

	struct ghttp__request request = {0};
	if (!ghttp__parse_request(&request, buf)) {
		warn("can't parse http request");
		goto done;
	}

	bool found = false;
	for (size_t i = 0; i < args->responder_count; i++) {
		const struct ghttp__path_responder* responder = &args->responders[i];
		bool is_avalidable = true;

		if (strcmp(request.type, responder->request_type) != 0) is_avalidable = false;
		if (responder->use_regex) {
			regex_t regex;
			if (regcomp(&regex, responder->path, 0) != 0) goto done;

			switch (regexec(&regex, request.url, 0, NULL, 0)) {
			case 0:
				break;

			case REG_NOMATCH:
				is_avalidable = false;
				break;

			default:	
				regfree(&regex);
				goto done;
			}

			regfree(&regex);
		} else {
			if (strcmp(responder->path, request.url) != 0) is_avalidable = false;
		}

		if (is_avalidable) {
			found = true;

			const struct ghttp__responce responce =
				responder->responder(args->server_context, &request);

			char* buf_out = ghttp__malloc(GHTTP_BUFFER_SIZE);
			size_t size = 0;
			ghttp__create_responce(&responce, buf_out, &size);

			send(args->client_fd, buf_out, size, 0);

			ghttp__free(buf_out);
		}
	}

	if (!found && args->not_found != NULL) {
		const struct ghttp__responce responce =
			args->not_found(args->server_context, &request);

		char* buf_out = ghttp__malloc(GHTTP_BUFFER_SIZE);
		size_t size = 0;
		ghttp__create_responce(&responce, buf_out, &size);

		send(args->client_fd, buf_out, size, 0);

		ghttp__free(buf_out);
	}

done:
	close(args->client_fd);
	ghttp__free_request(&request);
	ghttp__free(args);

	return NULL;
}

bool ghttp__send_request(char* host, int port, const struct ghttp__request* request,
		struct ghttp__responce* responce)
{
	bool return_code = false;

	int fd, server;
	struct sockaddr_in serv_addr = {0};

	char* buf = ghttp__malloc(GHTTP_BUFFER_SIZE);
	memset(buf, '\0', GHTTP_BUFFER_SIZE);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return false;

	if ((server = gethostname(host, strlen(host))) < 0)
		goto done;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port   = htons(port);

	if (connect(fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		goto done;

	size_t out_size;
	ghttp__create_request(request, buf, &out_size);
	send(fd, buf, out_size, 0);
	memset(buf, '\0', GHTTP_BUFFER_SIZE);

	size_t recv_size = recv(fd, buf, sizeof(buf), 0);
	if (!ghttp__parse_responce(responce, buf))
		goto done;

done:
	ghttp__free(buf);
	close(fd);

	return return_code;
}
