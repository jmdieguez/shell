#include "exec.h"

#define ERROR -1
#define OVERWRITE 1

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		char key[BUFLEN];
		char val[BUFLEN];
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], val, block_contains(eargv[i], '='));
		if (setenv(key, val, 1) == ERROR)
			fprintf_debug(
			        stderr,
			        "Error while setting environment variable\n");
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (flags & O_CREAT)
		return open(file, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	return open(file, flags);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option

void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);
		if (execvp(e->argv[0], e->argv) == ERROR) {
			fprintf_debug(stderr,
			              "Command '%s' not found\n",
			              e->argv[0]);
			_exit(ERROR);
		}

		exit(0);
		break;
	}

	case BACK: {
		b = (struct backcmd *) cmd;
		e = (struct execcmd *) (b->c);

		if (execvp(e->argv[0], e->argv) == ERROR) {
			fprintf_debug(stderr, "Error while exec\n");
			_exit(2);
		}

		exit(0);
		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;

		if (strlen(r->in_file) > 0) {
			int in_fd = open_redir_fd(r->in_file, O_RDONLY);

			if (in_fd > 0) {
				dup2(in_fd, STDIN_FILENO);
				close(in_fd);
			}

			else
				_exit(ERROR);
		}

		if (strlen(r->out_file) > 0) {
			int out_fd = open_redir_fd(r->out_file,
			                           O_WRONLY | O_CREAT | O_TRUNC);

			if (out_fd > 0) {
				dup2(out_fd, STDOUT_FILENO);
				close(out_fd);
			}

			else
				_exit(ERROR);
		}

		if (strlen(r->err_file) > 0) {
			int err_fd;

			if (r->err_file[0] == '&')
				err_fd = atoi(r->err_file +
				              1);  // err_file trated as fd

			else
				err_fd = open_redir_fd(
				        r->err_file,  // open err file
				        O_WRONLY | O_CREAT | O_TRUNC);

			if (err_fd > 0)
				dup2(err_fd, STDERR_FILENO);
			else
				_exit(ERROR);
		}

		if (execvp(r->argv[0], r->argv) == ERROR) {
			fprintf_debug(stderr, "Error while exec \n");
			_exit(ERROR);
		}

		exit(0);
		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;
		int wstatus;

		int fd[2];
		if (pipe(fd) == ERROR) {
			fprintf_debug(stderr, "Error while creating pipes\n");
			_exit(ERROR);
		}

		int fork_l = fork();
		if (fork_l < 0) {  // Error on left cmd fork
			close(fd[WRITE]);
			close(fd[READ]);
			fprintf_debug(stderr, "Error while forking\n");
			_exit(ERROR);
		}

		if (fork_l == 0) {  // Exec leftcmd
			free_command(p->rightcmd);
			free((struct pipecmd *) cmd);
			close(fd[READ]);
			dup2(fd[WRITE], STDOUT_FILENO);
			close(fd[WRITE]);
			exec_cmd(p->leftcmd);
		}

		int fork_r = fork();
		if (fork_r < 0) {  // Error on right cmd fork
			close(fd[WRITE]);
			close(fd[READ]);
			fprintf_debug(stderr, "Error while forking\n");
		}

		if (fork_r == 0) {  // Exec rightcmd
			free_command(p->leftcmd);
			free((struct pipecmd *) cmd);
			close(fd[WRITE]);
			dup2(fd[READ], STDIN_FILENO);
			close(fd[READ]);
			exec_cmd(p->rightcmd);
		}

		close(fd[WRITE]);
		close(fd[READ]);

		waitpid(fork_l, &wstatus, 0);
		waitpid(fork_r, &wstatus, 0);

		free_command(cmd);

		exit(WEXITSTATUS(wstatus));
		break;
	}
	}
}