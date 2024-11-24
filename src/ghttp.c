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

#include "ghttp.h"

#define BRBR "\r\n"

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

	char buf[HTTP_BUFFER_SIZE] = {0};
	vsprintf(buf, fmt, args);

	va_end(args);

	die(buf);
}

static void warnf(const char* fmt, ...)
{
	va_list	args;
	va_start(args, fmt);

	char buf[HTTP_BUFFER_SIZE] = {0};
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
		struct ghttp__reciver_args* http_rec_args = malloc(sizeof(struct ghttp__reciver_args));
		
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

static bool ghttp__parse_request_type(enum ghttp__request_type* type, const char* str)
{
	if (strcmp(str, "GET") == 0) 	   *type = HTTPRQT_GET;
	else if (strcmp(str, "POST") == 0) *type = HTTPRQT_POST;
	else goto fail;

	return true;
fail:
	return false;
}

static bool ghttp__create_request_type(const enum ghttp__request_type* type, const char** str)
{
	static const char* get_name  = "GET";
	static const char* post_name = "POST";

	switch (*type) {
	case HTTPRQT_GET:
		*str = get_name;
		break;

	case HTTPRQT_POST:
		*str = post_name;
		break;

	default:
		goto fail;
	}

	return true;
fail:
	return false;
}

static void ghttp__add_field(char* buf, const char* name, const char* val)
{
	strcat(buf, name);
	strcat(buf, ": ");
	strcat(buf, val);
	strcat(buf, BRBR);
}

static bool ghttp__parse_field(const char* str, char* name, char* value)
{
	char buf[HTTP_BUFFER_SIZE] = {0};
	strcpy(buf, str);

	char* save_ptr = buf;

	const char* name_ptr  = strtok_r(save_ptr, ":", &save_ptr);
	const char* value_ptr = strtok_r(save_ptr, "", &save_ptr);

	if (name_ptr == NULL || value_ptr == NULL) return false;

	strcpy(name, name_ptr);
	strcpy(value, value_ptr);

	return true;
}

static void ghttp__create_content_fields(const struct ghttp__content* content, char* buf)
{
	char temp[HTTP_BUFFER_SIZE] = {0};

	sprintf(temp, "%zu", content->content_size);
	ghttp__add_field(buf, "Content-Length", temp);
	ghttp__add_field(buf, "Content-Type", content->content_type);
}

static int ghttp__parse_content(struct ghttp__content* content, const char* content_str)
{
	char buf[HTTP_BUFFER_SIZE] = {0};
	strcpy(buf, content_str);

	char* save_ptr;
	strtok_r(buf, BRBR, &save_ptr);

	bool content_type_has = false;
	bool content_size_has = false;
	for (char* s = strtok_r(NULL, BRBR, &save_ptr);
			s != NULL && strcmp(s, "") != 0; s = strtok_r(NULL, BRBR, &save_ptr)) {
		char name[64]  = {0};
		char val[1024] = {0};
		if (!ghttp__parse_field(s, name, val)) return 1;

		if (strcmp(name, "Content-Length") == 0) {
			content_size_has = true;
			if (sscanf(val, "%zu", &content->content_size) != 1) return 1;
		}

		if (strcmp(name, "Content-Type") == 0) {
			content_type_has = true;
			strcpy(content->content_type, name);
		}
	}

	if (!content_size_has || !content_type_has) return 2;

	memcpy(content->content, strtok_r(save_ptr, BRBR, &save_ptr), content->content_size);

	return 0;
}

static bool ghttp__parse_request(struct ghttp__request* request, const char* request_str)
{
	char temp[HTTP_BUFFER_SIZE] = {0};
	char buf[HTTP_BUFFER_SIZE]  = {0};
	strcpy(buf, request_str);

	char* save_ptr;
	char* s = strtok_r(buf, BRBR, &save_ptr);

	if (sscanf(s, "%s %s HTTP/1.1", temp, request->url) != 2) return false;
	if (!ghttp__parse_request_type(&request->type, temp))     return false;	

	int content_get_code = ghttp__parse_content(&request->content.d, request_str);
	if (content_get_code == 1) return false;
	request->content.h = !(content_get_code == 2);

	bool host_has = false;
	for (s = strtok_r(NULL, BRBR, &save_ptr);
			s != NULL && strcmp(s, "") != 0; s = strtok_r(NULL, BRBR, &save_ptr)) {
		char name[64]  = {0};
		char val[1024] = {0};
		if (!ghttp__parse_field(s, name, val)) return false;

		if (strcmp(name, "Host") == 0) {
			host_has = true;
			strcpy(request->host, val);
		}
	}

	return host_has;
}

static bool ghttp__create_request(const struct ghttp__request* request, char* buf, size_t* size)
{
	size_t out_size = 0;

	const char* request_type;
	if (!ghttp__create_request_type(&request->type, &request_type))
		return false;

	sprintf(buf, "%s %s HTTP/1.1" BRBR, request_type, request->url);
	
	ghttp__add_field(buf, "Host", request->host);
	if (request->content.h) ghttp__create_content_fields(&request->content.d, buf);

	strcat(buf, BRBR);
	out_size += strlen(buf);

	if (request->content.h) {
		memcpy(buf + out_size, request->content.d.content, request->content.d.content_size);
		out_size += request->content.d.content_size;
	}

	*size = out_size;

	return true;
}

static bool ghttp__parse_responce(struct ghttp__responce* responce, const char* responce_str)
{
	return true;
}

static void ghttp__create_responce(const struct ghttp__responce* responce, char* buf, size_t* size)
{
	size_t out_size = 0;

	sprintf(buf, "HTTP/1.1 %i %s" BRBR, responce->responce_code, responce->responce_str);
	
	ghttp__add_field(buf, "Server", responce->server);
	if (responce->content.h) ghttp__create_content_fields(&responce->content.d, buf);

	strcat(buf, BRBR);
	out_size += strlen(buf);

	if (responce->content.h) {
		memcpy(buf + out_size, responce->content.d.content, responce->content.d.content_size);
		out_size += responce->content.d.content_size;
	}

	*size = out_size;
}

static void* ghttp__reciver(struct ghttp__reciver_args* args)
{
	char buf[HTTP_BUFFER_SIZE];
	ssize_t recv_size = recv(args->client_fd, buf, HTTP_BUFFER_SIZE, 0);

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

		if (request.type != responder->request_type) is_avalidable = false;
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
				responder->responder(args->server_context, request);

			char buf_out[HTTP_BUFFER_SIZE] = {0};
			size_t size = 0;
			ghttp__create_responce(&responce, buf_out, &size);

			send(args->client_fd, buf_out, size, 0);
		}
	}

	if (!found && args->not_found != NULL) {
		const struct ghttp__responce responce =
			args->not_found(args->server_context, request);

		char buf_out[HTTP_BUFFER_SIZE] = {0};
		size_t size = 0;
		ghttp__create_responce(&responce, buf_out, &size);

		send(args->client_fd, buf_out, size, 0);
	}

done:
	close(args->client_fd);
	free(args);

	return NULL;
}

bool ghttp__send_request(char* host, int port, const struct ghttp__request* request,
		struct ghttp__responce* responce)
{
	bool return_code = false;

	int fd, server;
	struct sockaddr_in serv_addr = {0};
	char buf[HTTP_BUFFER_SIZE]   = {0};

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
	memset(buf, '\0', sizeof(buf));

	size_t recv_size = recv(fd, buf, sizeof(buf), 0);
	if (!ghttp__parse_responce(responce, buf))
		goto done;

done:
	close(fd);
	return return_code;
}
