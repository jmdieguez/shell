#include "builtin.h"
#include <string.h>
#include "utils.h"

const char *histfile;

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)

int
exit_shell(char *cmd)
{
	char *aux = strdup(cmd);

	char s[2] = " ";
	char *token = strtok(aux, s);
	if (strcmp(token, "exit") == 0) {
		return PERFORMED;
	}

	return UNPERFORMED;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']

void
update_prompt(char *directory, char *curdir)
{
	char aux[FNAMESIZE] = "(";
	if (strcmp(directory, "..")) {
		strcat(aux, curdir);
		strcat(aux, "/");
		strcat(aux, directory);
	}

	else {
		for (int i = (FNAMESIZE - 1); i > -1; i--) {
			if (curdir[i] == '/') {
				strncat(aux, curdir, i);
				break;
			}
		}
	}

	strcat(aux, ")");
	strcpy(prompt, aux);
}

int
cd(char *cmd)
{
	char *aux = strdup(cmd);
	const char s[2] = " ";
	char *token = strtok(aux, s);
	if (strcmp(token, "cd") == 0) {
		char *directory = strtok(NULL, s);
		char curdir[FNAMESIZE];

		if (!getcwd(curdir, sizeof curdir)) {
			fprintf_debug(stderr,
			              "getcwd: %s: %s\n",
			              strerror(errno),
			              curdir);
			return PERFORMED;
		}

		if (!directory) {
			directory = getenv("HOME");
		}

		if (chdir(directory)) {
			fprintf_debug(stderr,
			              "chdir: %s: %s\n",
			              strerror(errno),
			              directory);
			return PERFORMED;
		}

		update_prompt(directory, curdir);

		return PERFORMED;
	}

	return UNPERFORMED;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		char curdir[FNAMESIZE];
		printf("%s\n", getcwd(curdir, sizeof curdir));
		return PERFORMED;
	}
	return UNPERFORMED;
}

char *history[HISTORY_MAX_SIZE];
unsigned history_count = 0;

void
add_command_to_history(const char *cmd)
{
	if (history_count < HISTORY_MAX_SIZE) {
		history[history_count++] = strdup(cmd);
	} else {
		free(history[0]);
		for (unsigned index = 1; index < HISTORY_MAX_SIZE; index++) {
			history[index - 1] = history[index];
		}
		history[HISTORY_MAX_SIZE - 1] = strdup(cmd);
	}
}

void
load_history(void)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	fp = fopen(histfile, "r");
	if (fp == NULL)
		return;
	while (getline(&line, &len, fp) != -1) {
		strtok(line, "\n");
		add_command_to_history(line);
	}

	fclose(fp);
	if (line)
		free(line);
}
void
write_history(void)
{
	int fd;
	fd = open(histfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (!fd)
		return;

	for (unsigned i = 0; i < history_count; i++) {
		strcat(history[i], "\n");
		if (write(fd, history[i], strlen(history[i])) < 0)
			exit(1);
		free(history[i]);
	}

	close(fd);
}

int
print_history(char *cmd)
{
	char *aux = strdup(cmd);
	const char s[2] = " ";
	char *token = strtok(aux, s);

	if (strcmp(token, "history") == 0) {
		int i = 0;

		char *n = strtok(NULL, s);
		if (n) {
			i = history_count - atoi(n);
			if (i < 0)
				i = 0;
		}

		while ((unsigned) i < history_count) {
			printf("%5d  %s\n", i + 1, history[i]);
			i++;
		}

		return PERFORMED;
	}

	return UNPERFORMED;
}