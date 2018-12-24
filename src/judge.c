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

typedef struct {
	char *name;
	char **solutions;
	int *points;
} Participant;

DIR *s_opendir(char *path);
void s_chdir(char *path);
int s_open(char *fileName, int flag);
void *s_realloc(void *ptr, int size);
int s_close(int fd);
int s_dup2(int oldfd, int newfd);

char **get_solutions(DIR *sol_dir, Participant *prt)
{
	char **solutions = NULL, *tmp;
	struct dirent *entry;
	int i = 0, max_len = 1, *points;

	while ((entry = readdir(sol_dir)) != NULL) {
		if (i + 1 >= max_len) {
			max_len <<= 1;
			solutions = s_realloc(solutions,
					max_len * sizeof(char *));
			points = s_realloc(points,
					max_len * sizeof(int *));
		}
		points[i] = 0;
		tmp = entry->d_name;
		if (strncmp(tmp, ".", 2) && strncmp(tmp, "..", 3)) //add filter
			solutions[i++] = tmp;
	}
	if (i == 0)
		free(solutions);
	solutions[i] = NULL;
	prt->solutions = solutions;
	prt->points = points;
	return solutions;
}

Participant *get_participants_list(void)
{
	int i = 0, max_len = 1;
	struct dirent *entry;
	char *tmp;
	DIR *participants_dir = s_opendir("participants"), *sol_dir;
	Participant *participants = NULL;

	s_chdir("participants");
	while ((entry = readdir(participants_dir)) != NULL) {
		if (i + 1 >= max_len) {
			max_len <<= 1;
			participants = s_realloc(participants,
					max_len * sizeof(Participant));
		}
		sol_dir = opendir(entry->d_name);
		if (sol_dir != NULL) {
			participants[i].name = (tmp = entry->d_name);
			if (strncmp(tmp, ".", 2) && strncmp(tmp, "..", 3)) {
				participants[i].points = 0;
				get_solutions(sol_dir, participants + i);
				++i;
			}
		} else {
			perror("opendir failed");
		}
	}
	if (i == 0) {
		perror("No participants");
		free(participants);
		exit(-1);
	}
	participants[i].name = NULL;
	s_chdir("..");
	closedir(participants_dir);
	return participants;
}

void remove_extension(char *s, int max)
{
	int length = strlen(s);

	for (int i = 1; i < max; ++i)
		if (s[length - i] == '.') {
			s[length - i] = 0;
			break;
		}

}

int testing(Participant *prts, char *contest)
{
	pid_t pid;
	int pipe_fd[2];
	char ch;

	for (int i = 0; prts[i].name != NULL; ++i) {
		for (int j = 0; prts[i].solutions[j] != NULL; ++j) {
			if (pipe(pipe_fd) < 0) {
				perror("pipe failed");
				continue;
			}
			pid = fork();
			if (pid < 0) {
				perror("fork failed, omiting");
				s_close(pipe_fd[0]);
				s_close(pipe_fd[1]);
			} else if (pid != 0) {
				s_close(pipe_fd[1]);
				while (ch != EOF) {
					if (read(pipe_fd[0], &ch, 1) < 0) {
						perror("read failed, omiting");
						kill(pid, SIGTERM);
						s_close(pipe_fd[0]);
						ch = EOF;
					}
					if (ch == '+')
						++prts[i].points[j];
				}
				waitpid(pid, NULL, 0);
			} else {
				char sol_path[NAME_MAX];
				char prblm_path[NAME_MAX];

				snprintf(sol_path, NAME_MAX - 1,
						"%s/participants/%s/%s",
						contest, prts[i].name,
						prts[i].solutions[j]);
				snprintf(prblm_path, NAME_MAX - 1,
						"%s/problems/%s", contest,
						prts[i].solutions[j]);
				remove_extension(prblm_path,
						strlen(prts[i].solutions[j]));
				s_close(pipe_fd[0]);
				s_dup2(1, pipe_fd[1]);
				if (execlp("tester", "tester",
						sol_path,
						prblm_path,
						contest,
						prts[i].name,
						NULL) < 0) {
					perror("Can't exec tester");
					exit(-1);
				}
			}
		}
	}
	return 0;
}

void init(int argc, char **argv, int *config)
{
	if (argc != 2) {
		perror("Wrong number of arguments");
		exit(-1);
	}
	s_chdir(argv[1]);
	*config = s_open("global.cfg", 0);
	if (*config < 0) {
		perror("Can't open global.cfg");
		exit(-1);
	}
}

int printResult(Participant *prt)
{
	for (int i = 0; prt[i].name != NULL; ++i) {
		printf("%s", prt[i].name);
		for (int j = 0; prt[i].solutions[j] != 0; ++j)
			printf(", %s %d",
				prt[i].solutions[j], prt[i].points[j]);
		puts("");
	}
	return 0;
}

int main(int argc, char **argv)
{
	int config;
	Participant *participants;

	init(argc, argv, &config);
	participants = get_participants_list();
	testing(participants, argv[1]);
	printResult(participants);
	free(participants);
	s_close(config);
	return 0;
}
