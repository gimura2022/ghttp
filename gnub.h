/* include part */

#ifndef _gnub_h
#define _gnub_h

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define GNUB_MAX_CMD_PART_LENGHT 1024
#define GNUB_MAX_CMD_LEN 1024
#define GNUB_MAX_FILE_NAME 64
#define GNUB_MAX_TARGET_NAME 64
#define GNUB_MAX_TARGETS 32

#define GNUB_FIND_C_FILES_MAX_FILES 64

#define array_lenght(x) sizeof(x) / sizeof(x[0])

struct _gnub__cmd_part {
	struct _gnub__cmd_part* next;
	char str[GNUB_MAX_CMD_PART_LENGHT];
};

struct _gnub__cmd {
	struct _gnub__cmd* next;

	struct _gnub__cmd_part* start;
	struct _gnub__cmd_part* end;
};

struct gnub__cmd_arr {
	struct _gnub__cmd* start;
	struct _gnub__cmd* end;
};

typedef void (*gnub__target_hendler_t)(void);

struct _gnub__target {
	char name[GNUB_MAX_TARGET_NAME];
	gnub__target_hendler_t handler;
};

/* private functions */

void _gnub__append_command(struct gnub__cmd_arr* arr, const size_t count, const char** parts);
void _gnub__append_parts_to_last(struct gnub__cmd_arr* arr, const size_t count, const char** parts);
void _gnub__append_parts_by_index(struct gnub__cmd_arr* arr, const size_t count, const char** parts);

/* public functions */

int gnub__execute_commands(struct gnub__cmd_arr* cmds);
void gnub__free_commands(struct gnub__cmd_arr* cmds);

bool gnub__find_c_files(const char* path, char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME],
		size_t* count);

void gnub__create_executable(struct gnub__cmd_arr* arr, const char* ld, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count);

void gnub__create_static_lib(struct gnub__cmd_arr* arr, const char* ar, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count);
void gnub__create_shared_lib(struct gnub__cmd_arr* arr, const char* cc, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count);
void gnub__create_lib(struct gnub__cmd_arr* arr, const char* ar, const char* cc, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count);

void gnub__install_lib(struct gnub__cmd_arr* arr, const char* name, const char* prefix, int type,
		const char* include);

bool gnub__recompile_self_with_build_arr(struct gnub__cmd_arr* arr, const char* output_file, char* argv[]);
bool gnub__recompile_self(char* argv[]);
bool gnub__compile_subproject(const char* path, char* argv[]);

void gnub__add_target(const char* name, gnub__target_hendler_t handler);
void gnub__run_targets(int argc, char* argv[], const char* defaults_target[], const size_t count);

char* gnub__get_env_variable(char* name, char* or_default);

#define _gnub__parts_command(x, arr, ...) ({ const char* __parts[] = {__VA_ARGS__}; \
		x(arr, array_lenght(__parts), __parts); })

#define gnub__append_command(arr, ...) _gnub__parts_command(_gnub__append_command, arr, __VA_ARGS__)
#define gnub__append_parts_to_last(arr, ...) _gnub__parts_command(_gnub__append_parts_to_last, \
		arr, __VA_ARGS__)
#define gnub__append_parts_by_index(arr, ...) _gnub__parts_command(_gnub__append_parts_by_index, \
		arr, __VA_ARGS__)

#endif

/* implementation part */

#ifdef gnub_impl

/* private functions */

static void _gnub__append_to_command(struct _gnub__cmd* cmd, const size_t count, const char** parts)
{
	for (size_t i = 0; i < count; i++) {
		memcpy(cmd->end->str, parts[i], strlen(parts[i]));

		cmd->end->next = (struct _gnub__cmd_part*) malloc(sizeof(struct _gnub__cmd_part));
		cmd->end       = cmd->end->next;
		cmd->end->next = NULL;
		memset(cmd->end->str, '\0', sizeof(cmd->end->str));
	}
}

void _gnub__append_command(struct gnub__cmd_arr* arr, const size_t count, const char** parts)
{
	bool is_first_cmd = false;

	if (arr->start == NULL) {
		is_first_cmd = true;

		arr->start     = (struct _gnub__cmd*) malloc(sizeof(struct _gnub__cmd));
		arr->end       = arr->start;
		arr->end->next = NULL;
	}

	if (!is_first_cmd) {
		arr->end->next = (struct _gnub__cmd*) malloc(sizeof(struct _gnub__cmd));
		arr->end       = arr->end->next;
		arr->end->next = NULL;
	}

	arr->end->start     = (struct _gnub__cmd_part*) malloc(sizeof(struct _gnub__cmd_part));
	arr->end->end       = arr->end->start;
	arr->end->end->next = NULL;
	memset(arr->end->end->str, '\0', sizeof(arr->end->end->str));

	_gnub__append_to_command(arr->end, count, parts);
}

static int _gnub__execute_command(struct _gnub__cmd* cmd)
{
	char out_command[GNUB_MAX_CMD_PART_LENGHT];
	memset(out_command, '\0', sizeof(out_command));

	struct _gnub__cmd_part* part = cmd->start;
	while (part != NULL) {
		strcat(out_command, part->str);	
		strcat(out_command, " ");

		part = part->next;
	}

	printf("%s\n", out_command);
	return system(out_command);
}

static void _gnub__free_command(struct _gnub__cmd* cmd)
{
	struct _gnub__cmd_part* part = cmd->start;
	while (part != NULL) {
		struct _gnub__cmd_part* next = part->next;
		free(part);
		part = next;
	}

	free(cmd);
}

void _gnub__append_parts_to_last(struct gnub__cmd_arr* arr, const size_t count, const char** parts)
{
	_gnub__append_to_command(arr->end, count, parts); // TODO: add check to empty command array
}

void _gnub__append_parts_by_index(struct gnub__cmd_arr* arr, const size_t count, const char** parts)
{
	size_t i               = 0;
	struct _gnub__cmd* cmd = arr->start;
	while (i < count && cmd != NULL) {
		i++;
		cmd = cmd->next;
	}

	_gnub__append_to_command(cmd, count, parts);
}

static bool _gnub__compare_files(const char* file0, const char* file1)
{
	FILE* file0_ds = fopen(file0, "rb");
	FILE* file1_ds = fopen(file1, "rb");

	int c0 = fgetc(file0_ds);
	int c1 = fgetc(file1_ds);

	bool is_eq = false;

	while (c0 != EOF && c1 != EOF) {
		if (c0 != c1) goto done;	

		c0 = fgetc(file0_ds);
		c1 = fgetc(file1_ds);
	}

	if (c0 == EOF && c1 == EOF) {
		is_eq = true;
		goto done;
	}

done:
	fclose(file0_ds);
	fclose(file1_ds);

	return is_eq;
}

/* public functions */

int gnub__execute_commands(struct gnub__cmd_arr* cmds)
{
	struct _gnub__cmd* cmd = cmds->start;
	while (cmd != NULL) {
		int code = _gnub__execute_command(cmd);
		if (code != 0) {
			return code;
		}

		cmd = cmd->next;
	}

	return 0;
}

void gnub__free_commands(struct gnub__cmd_arr* cmds)
{
	struct _gnub__cmd* cmd = cmds->start;
	while (cmd != NULL) {
		struct _gnub__cmd* next = cmd->next;
		_gnub__free_command(cmd);
		cmd = next;
	}
}

bool gnub__recompile_self_with_build_arr(struct gnub__cmd_arr* arr, const char* output_file, char* argv[])
{
	if (strcmp(output_file, argv[0]) == 0) return false;
	if (gnub__execute_commands(arr) != 0) return false;
	if (_gnub__compare_files(output_file, argv[0])) {
		remove(output_file);
		return true;
	}

	gnub__free_commands(arr);

	remove(argv[0]);
	rename(output_file, argv[0]);

	execv(argv[0], argv);
	exit(0);
}

bool gnub__recompile_self(char* argv[])
{
	struct gnub__cmd_arr cmds = {0};

	char output_file[32] = {0};
	strcpy(output_file, argv[0]);
	strcat(output_file, ".new");

	gnub__append_command(&cmds, "cc", "-o", output_file, "gnub.c");

	bool result = gnub__recompile_self_with_build_arr(&cmds, output_file, argv);
	gnub__free_commands(&cmds);

	return result;
}

bool gnub__find_c_files(const char* path, char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME],
		size_t* count)
{
	DIR* dir = opendir(path);
	struct dirent* entry;
	size_t i = 0;

	if (dir == NULL) return false;	

	while ((entry = readdir(dir)) != NULL) {
		char file_name[64] = {0};
		strcat(file_name, path);
		strcat(file_name, entry->d_name);

		if (entry->d_type == DT_REG &&
				strcmp(file_name + strlen(file_name) - 2, ".c") == 0) {
			char output_file[64] = {0};
			strcat(output_file, file_name);
			strcat(output_file, ".o");

			strcpy(output[i][0], file_name);
			strcpy(output[i][1], output_file);
			i++;
		}
	}

	*count = i;

	closedir(dir);

	return true;
}

void gnub__create_executable(struct gnub__cmd_arr* arr, const char* ld, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count)
{
	char files_str[1024] = {0};
	for (int i = 0; i < count; i++) {
		strcat(files_str, output[i][1]);
		strcat(files_str, " ");
	}

	gnub__append_command(arr, ld, "-o", name, files_str, ldflags);
}

void gnub__create_static_lib(struct gnub__cmd_arr* arr, const char* ar, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count)
{
	char files_str[1024] = {0};
	for (int i = 0; i < count; i++) {
		strcat(files_str, output[i][1]);
		strcat(files_str, " ");
	}

	char lib_name[GNUB_MAX_FILE_NAME] = {0};
	strcat(lib_name, "lib");
	strcat(lib_name, name);
	strcat(lib_name, ".a");

	gnub__append_command(arr, ar, "rcs", lib_name, files_str);
}

void gnub__create_shared_lib(struct gnub__cmd_arr* arr, const char* cc, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count)
{
	char files_str[1024] = {0};
	for (int i = 0; i < count; i++) {
		strcat(files_str, output[i][1]);
		strcat(files_str, " ");
	}
	
	char lib_name[GNUB_MAX_FILE_NAME] = {0};
	strcat(lib_name, "lib");
	strcat(lib_name, name);
	strcat(lib_name, ".so");

	gnub__append_command(arr, cc, "-shared", "-o", lib_name, files_str, ldflags);
}

void gnub__create_lib(struct gnub__cmd_arr* arr, const char* ar, const char* cc, const char* name,
		const char* ldflags,
		char output[GNUB_FIND_C_FILES_MAX_FILES][2][GNUB_MAX_FILE_NAME], const size_t count)
{
	gnub__create_shared_lib(arr, cc, name, ldflags, output, count);
	gnub__create_static_lib(arr, ar, name, ldflags, output, count);
}

bool gnub__compile_subproject(const char* path, char* argv[])
{
	char path_to_gnub[GNUB_MAX_FILE_NAME] = {0};
	strcat(path_to_gnub, "./");
	strcat(path_to_gnub, path);
	strcat(path_to_gnub, "/gnub");

	struct stat tmp;
	if (stat(path_to_gnub, &tmp) == -1) {
		char path_to_gnub_c[GNUB_MAX_FILE_NAME] = {0};
		strcpy(path_to_gnub_c, path_to_gnub);
		strcat(path_to_gnub_c, ".c");

		struct gnub__cmd_arr arr = {0};
		gnub__append_command(&arr, "cc", "-o", path_to_gnub, path_to_gnub_c);

		gnub__execute_commands(&arr);
		gnub__free_commands(&arr);
	}

	char old_dir[GNUB_MAX_FILE_NAME] = {0};
	getcwd(old_dir, sizeof(old_dir));

	chdir(path);
	system("./gnub");
	chdir(old_dir);
}

void gnub__install_lib(struct gnub__cmd_arr* arr, const char* name, const char* prefix, int type,
		const char* include)
{
	char libpath[GNUB_MAX_FILE_NAME] = {0};
	strcat(libpath, prefix);
	strcat(libpath, "/lib");

	char includepath[GNUB_MAX_FILE_NAME] = {0};
	strcat(includepath, prefix);
	strcat(includepath, "/include");

	char all_includefiles[GNUB_MAX_FILE_NAME] = {0};
	strcat(all_includefiles, include);
	strcat(all_includefiles, "/*");

	char libname[GNUB_MAX_FILE_NAME] = {0};
	strcat(libname, "lib");
	strcat(libname, name);

	char libname_static[GNUB_MAX_FILE_NAME] = {0};
	strcat(libname_static, libname);
	strcat(libname_static, ".a");

	char libname_shared[GNUB_MAX_FILE_NAME] = {0};
	strcat(libname_shared, libname);
	strcat(libname_shared, ".so");

	gnub__append_command(arr, "install -d", libpath);
	switch (type) {
	case 0:	
	case 1:
		gnub__append_command(arr, "install -m 644", libname_static, libpath);
		if (type != 0) break;

	case 2:
		gnub__append_command(arr, "install -m 755", libname_shared, libpath);
	}

	gnub__append_command(arr, "install -d", includepath);
	gnub__append_command(arr, "install -m 644", all_includefiles, includepath);
}

static struct _gnub__target targets[GNUB_MAX_TARGETS];
static size_t targets_count = 0;

void gnub__add_target(const char* name, gnub__target_hendler_t handler)
{
	struct _gnub__target target = {0};
	strcpy(target.name, name);	
	target.handler = handler;

	targets[targets_count++] = target;
}

static void _gnub__run_target(const char* name)
{
	for (int i = 0; i < targets_count; i++) {
		if (strcmp(targets[i].name, name) == 0) targets[i].handler();
	}
}

void gnub__run_targets(int argc, char* argv[], const char* defaults_target[], const size_t count)
{
	if (argc == 1) {
		for (int i = 0; i < count; i++) {
			_gnub__run_target(defaults_target[i]);
		}
	}

	for (int i = 1; i < argc; i++) {
		_gnub__run_target(argv[i]);	
	}
}

char* gnub__get_env_variable(char* name, char* or_default)
{
	return getenv(name) == NULL ? or_default : getenv(name);
}

#endif
