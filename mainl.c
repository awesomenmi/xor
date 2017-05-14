#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#define SIZE_BUF 4096

int error_exit(int id_error)
{
	printf("Error! Comment: %s\n", strerror(errno));
	_exit(-1);
}

int main(int argc, char *argv[])
{
	char n_out[255], n_in[255], n_gamma[255];
	char *arguments[3];
	char *argument[3];
	int pipe_in[2], pipe_gamma[2];
	int f_out, f_gamma;
	int cnt_read_in, cnt_read_gamma;
	int i;

	strcpy(n_in, argv[2]);
	strcpy(n_gamma, argv[4]);
	if (strcmp(n_gamma, "newgamma") == 0)
		strcat(n_gamma, "_new");
	else
		strcpy(n_gamma, "newgamma");
	argument[0] = argv[1];
	arguments[0] = argv[3];
	argument[1] = argv[2];
	arguments[1] = argv[4];
	argument[2] = argv[5];
	arguments[2] = argv[5];

	if (pipe(pipe_in) == -1)
		error_exit(errno);

	pid_t pid_in = fork();

	switch (pid_in) {
	case -1:
			error_exit(errno);
	case 0:
			close(1);
			dup(pipe_in[1]);
			close(pipe_in[0]);
			close(pipe_in[1]);
			execvp(argv[1], argument);
			_exit(EXIT_FAILURE);
	case 1:
			break;
	}
	if (pipe(pipe_gamma) == -1)
		error_exit(errno);
	pid_t pid_gamma = fork();

	switch (pid_gamma) {
	case -1:
			error_exit(errno);
	case 0:
			close(1);
			dup(pipe_gamma[1]);
			close(pipe_gamma[0]);
			close(pipe_gamma[1]);
			execvp(argv[3], arguments);
			_exit(EXIT_FAILURE);
	case 1:
			break;
	}
	close(pipe_in[1]);
	close(pipe_gamma[1]);

	char buf_in[SIZE_BUF], buf_gamma[SIZE_BUF], buf_out[SIZE_BUF];

	strcpy(n_out, n_in);
	strcat(n_out, "_out");
	f_out = creat(n_out, 0777);
	if (!f_out)
		error_exit(errno);
	f_gamma = creat(n_gamma, 0777);
	if (!f_gamma)
		error_exit(errno);
	sleep(1);
	cnt_read_in = read(pipe_in[0], buf_in, SIZE_BUF);
	cnt_read_gamma = read(pipe_gamma[0], buf_gamma, SIZE_BUF);
	for (i = 0; i < cnt_read_in; i++)
		buf_out[i] = buf_in[i] ^ buf_gamma[i];
	write(f_out, buf_out, cnt_read_in);
	write(f_gamma, buf_gamma, cnt_read_in);
	close(f_out);
	close(f_gamma);
	_exit(EXIT_SUCCESS);
}
