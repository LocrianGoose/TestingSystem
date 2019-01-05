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
#include "s_functions.c"

void init(int argc, char **argv)
{
	if (argc != 3 && argc != 4) {
		printf("Error: Wrong number of arguments");
		exit(1);
	}
	s_chdir(argv[2]);
}

int main(int argc, char **argv)
{
	init(argc, argv);
	char *root_dir = calloc(NAME_MAX, 1);
	getcwd(root_dir, NAME_MAX);
	pid_t pid = fork();
	if (pid == 0) {
		execlp("rm", "rm", "./.tmp.out", "-f", NULL);
	} else {
		wait(NULL);
	}
	pid = fork();
	if (pid == 0) {
		execlp("gcc", "gcc", argv[1], "-o", ".tmp.out", NULL);
	} else {
		wait(NULL);
	}
	
	struct dirent *entry, *entry2;
	char *tmp, *tmp2;
	DIR *tests_dir = opendir(argv[2]), *tests_dir2;
	strcat(root_dir, "/checkers/checker_byte");
	if (tests_dir == NULL) {
		perror("opendir failed");
		exit(-1);
	}
	int fd[2];
	pipe(fd);
	dup2(fd[0], 0);
	close(fd[0]);
	while ((entry = readdir(tests_dir)) != NULL) {
		if(strlen(tmp = entry->d_name) >= 4 && strcmp(tmp + strlen(tmp) - 4, ".dat") == 0) {
			tests_dir2 = opendir(argv[2]);
			while ((entry2 = readdir(tests_dir2)) != NULL) {
				if(strlen(tmp2 = entry2->d_name) >= 4 && strcmp(tmp2 + strlen(tmp2) - 4, ".ans") == 0 &&
				   strlen(tmp) == strlen(tmp2) && strncmp(tmp, tmp2, strlen(tmp2) - 4) == 0) {
					char *data, *answer;
					data = calloc(3, 1);
					answer = calloc(3, 1);
					data[0] = answer[0] = '.';
					data[1] = answer[1] = '/';
					strcat(data, tmp);
					strcat(answer, tmp2);
					pid_t pid;
					pid = fork();
					if (pid == 0) {
						dup2(fd[1], 1);
						close(fd[1]);
						execlp(root_dir, root_dir, "./.tmp.out", data, answer, NULL);
					} else {
						//printf("%s %s %s [%c]\n", root_dir, data, answer, getchar());
						printf("%c", getchar());
					}
				}
			}
		}
	}
	close(fd[1]);
	pid = fork();
	if (pid == 0) {
		execlp("rm", "rm", "./.tmp.out", "-f", NULL);
	}
	return 0;
}
