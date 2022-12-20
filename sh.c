#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"
#include <termios.h>

char prompt[PRMTLEN] = { 0 };

struct termios saved_attributes;
struct termios tattr;

void
reset_input_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void
set_input_mode(void)
{
	/* Make sure stdin is a terminal. */
	if (!isatty(STDIN_FILENO)) {
		fprintf(stderr, "Not a terminal.\n");
		exit(EXIT_FAILURE);
	}

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr(STDIN_FILENO, &saved_attributes);
	atexit(reset_input_mode);

	/* Set the funny terminal modes. */
	tcgetattr(STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON | ECHO); /* Clear ICANON and ECHO. */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);

	setvbuf(stdin, NULL, _IONBF, 0);
}

// runs a shell command
static void
run_shell()
{
	char *cmd;

	load_history();

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	set_input_mode();

	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

extern char *histfile;

int
main(void)
{
	init_shell();

#ifdef HISTFILE
	histfile = getenv("HISTFILE");
#endif
#ifndef HISTFILE
	histfile = ".fisop_history";
#endif
	run_shell();

	reset_input_mode();

	write_history();

	return 0;
}
