// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stddef.h>

void init(int argc, char **argv, int *input, int *answer)
{
	if (argc != 4) {
		printf("X");
		exit(0);
	}
	*input = open(argv[2], O_RDONLY);
	if (*input < 0) {
		printf("X");
		exit(0);
	}
	*answer = open(argv[3], O_RDONLY);
	if (*answer < 0) {
		printf("X");
		close(*input);
		exit(0);
	}
	if (access(argv[1], X_OK) == -1) {
		printf("X");
		close(*input);
		close(*answer);
		exit(0);
	}
}

int main(int argc, char **argv)
{
	int input, answer;
	init(argc, argv, &input, &answer);
	int fd[2];
	pipe(fd);
	pid_t pid = fork();
	if (pid < 0) {
		printf("X");
		close(fd[0]);
		close(fd[1]);
		exit(0);
	} else if (pid) {
		close(fd[1]);
		dup2(fd[0], 0);
		close(fd[0]);
		char ch, ans_ch;
		while (!kill(pid, 0)) {
			ch = getchar();
			if (read(answer, &ans_ch, 1) != 0) {
				if (ch != ans_ch) {
					printf("-");
					kill(pid, SIGKILL);
					close(0);
					return 0;
				}
			} else {
				printf("-");
				kill(pid, SIGKILL);
				close(0);
				return 0;
			}
		}
		char ending[3] = {0, 0, 0};
		if (read(answer, &ans_ch, 3) <= 1 && (ending[0] == '\n' || ending[0] == 0)) {
			printf("+");
		} else {
			printf("-");
		}
		close(0);
	} else {
		close(fd[0]);
		dup2(input, 0);
		close(input);
		dup2(fd[1], 1);
		close(fd[1]);
		execlp(argv[1], argv[1], NULL);
		exit(0);
	}
	return 0;
} 
