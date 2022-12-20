#include "runcmd.h"

int status = 0;
struct cmd *parsed_pipe;

extern char *history[HISTORY_MAX_SIZE];
extern unsigned history_count;

// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	if (cmd[0] == END_STRING)
		return 0;

	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the prompt again

	if (cmd[0] == '!' && history_count > 0) {
		if (cmd[1] == '!')
			strcpy(cmd, history[history_count - 1]);

		else if (cmd[1] == '-' && isdigit(cmd[2])) {
			int digit = cmd[2] - '0';
			if (digit > 0)
				strcpy(cmd, history[history_count - digit]);
		} else {
			fprintf(stdout, "event not found\n");
			return 0;
		}
	}

	if (history_count > 0) {
		if (strcmp(history[history_count - 1], cmd) != 0)
			add_command_to_history(cmd);
	} else
		add_command_to_history(cmd);

	if (print_history(cmd)) {
		return 0;
	}

	// "cd" built-in call
	if (cd(cmd)) {
		return 0;
	}

	if (exit_shell(cmd)) {
		return EXIT_SHELL;
	}

	if (pwd(cmd))
		return 0;

	parsed = parse_line(cmd);
	// forks and run the command
	if ((p = fork()) == 0) {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == PIPE)
			parsed_pipe = parsed;
		exec_cmd(parsed);
	}

	// stores the pid of the process
	parsed->pid = p;

	// background process special treatment
	// Hint:
	// - check if the process is
	//		going to be run in the 'back'
	// - print info about it with
	// 	'print_back_info()'
	//
	// Your code here

	// waits for the process to finish

	if (parsed->type == BACK)
		print_back_info(parsed);
	else {
		waitpid(p, &status, 0);
		print_status_info(parsed);
	}

	while (waitpid(-1, NULL, WNOHANG) > 0)
		;

	free_command(parsed);

	return 0;
}
