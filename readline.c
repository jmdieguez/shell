#include "defs.h"
#include "readline.h"
#include "runcmd.h"

#define BACKSPACE 0x7f
#define EOT 4
#define ESC '\033'

static char buffer[BUFLEN];
extern char *history[HISTORY_MAX_SIZE];
extern unsigned history_count;
int actual_cmd;

void
clean_input(int i)
{
	memset(buffer, 0, BUFLEN);  // clean buffer
	for (int a = 0; a < i; a++) {
		write(STDIN_FILENO, "\b \b", 3);  // clean input
	}
	fseek(stdin, 0, SEEK_END);
}

int
get_next_cmd(void)
{
	int i = 0;

	if (actual_cmd < history_count - 1) {
		char *aux = history[++actual_cmd];
		for (size_t j = 0; j < strlen(aux); j++)
			buffer[i++] = aux[j];
		write(STDIN_FILENO, aux, i);
	} else
		actual_cmd = history_count;
	return i;
}

int
get_previous_cmd(void)
{
	int i = 0;

	if (actual_cmd > 0) {
		char *aux = history[--actual_cmd];
		for (size_t j = 0; j < strlen(aux); j++)
			buffer[i++] = aux[j];
		write(STDIN_FILENO, aux, i);
	} else
		actual_cmd = history_count;
	return i;
}


// reads a line from the standard input
// and prints the prompt
char *
read_line(const char *prompt)
{
	int i = 0, c = 0;
	actual_cmd = history_count;

#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
#endif

	memset(buffer, 0, BUFLEN);
	c = getchar();
	while (c != END_LINE && c != EOF && c != EOT) {
		if (c == ESC) {  // if the first value is esc
			clean_input(i);
			i = 0;
			getchar();            // skip the [
			switch (getchar()) {  // the real value
			case KEYUP:
				i = get_previous_cmd();
				break;
			case KEYDOWN:
				i = get_next_cmd();
				break;
			}
		}

		else if (c == BACKSPACE) {                // if backspace
			write(STDIN_FILENO, "\b \b", 3);  // clean input
			buffer[i--] = '\0';
		}

		else {
			buffer[i++] = c;
			write(STDIN_FILENO, &c, 1);
		}

		c = getchar();
	}

	// if the user press ctrl+D
	// just exit normally
	if (c == EOF || c == EOT)
		return NULL;

	buffer[i] = END_STRING;

	write(STDIN_FILENO, "\n", 1);

	return buffer;
}
