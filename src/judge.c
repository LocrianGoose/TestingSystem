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
#include <sys/file.h>

#define SUMM 284
#define PERF 220

typedef struct {
	char *name;
	char **solutions;
	int *points;
} Participant;

DIR *s_opendir(char *path);
int s_open(char *fileName, int flag);
void *s_realloc(void *ptr, int size);
int s_close(int fd);
int s_dup2(int oldfd, int newfd);

int score_parameter = SUMM;
char problems_string[NAME_MAX] = "";

char **get_solutions(DIR *sol_dir, Participant *prt)
{
	char **solutions = NULL, *tmp;
	struct dirent *entry;
	int i = 0, max_len = 1, *points = NULL;

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
		if (strncmp(tmp, ".", 2) && strncmp(tmp, "..", 3))
			solutions[i++] = tmp;
	}
	solutions[i] = NULL;
	prt->solutions = solutions;
	prt->points = points;
	return solutions;
}

Participant *get_participants_list(char *contest)
{
	char path[NAME_MAX];
	char sol_d[2 * NAME_MAX + 1];
	int i = 0, max_len = 1;
	struct dirent *entry;
	char *tmp;
	DIR *participants_dir, *sol_dir;
	Participant *participants = NULL;

	snprintf(path, NAME_MAX - 1,
			"%s/participants",
			contest);
	participants_dir = s_opendir(path);
	while ((entry = readdir(participants_dir)) != NULL) {
		if (i + 1 >= max_len) {
			max_len <<= 1;
			participants = s_realloc(participants,
					max_len * sizeof(Participant));
		}
		snprintf(sol_d, 2 * NAME_MAX + 1, "%s/%s", path, entry->d_name);
		sol_dir = opendir(sol_d);
		if (sol_dir != NULL) {
			tmp = entry->d_name;
			participants[i].name = malloc(strlen(tmp) * sizeof(char));
			strncpy(participants[i].name, tmp, strlen(tmp));
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

int countScore(int fd, pid_t pid)
{
	char ch = '0';
	int score = 0;

	if (score_parameter == PERF)
		score = 1;
	while (ch != EOF) {
		if (read(fd, &ch, 1) <= 0)
			ch = EOF;
		switch (score_parameter) {
		case SUMM:
			if (ch == '+')
				++score;
			break;
		case PERF:
			if (ch == '-' || ch == 'X')
				score = 0;
			break;
		}
	}
	s_close(fd);
	waitpid(pid, NULL, 0);
	return score;
}

int testing(Participant *prts, char *contest)
{
	pid_t pid;
	int pipe_fd[2];

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
				prts[i].points[j] = countScore(pipe_fd[0], pid);
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
				s_dup2(pipe_fd[1], 1);
				if (execlp("./tester", "tester",
						sol_path,
						prblm_path,
						NULL) < 0) {
					perror("Can't exec tester");
					exit(-1);
				}
			}
		}
	}
	return 0;
}

void init(int argc, char **argv)
{
	int config_fd;
	char path[NAME_MAX];
	char prop_string[NAME_MAX];

	if (argc != 2) {
		perror("Wrong number of arguments");
		exit(-1);
	}

	snprintf(path, NAME_MAX - 1,
			"%s/global.cfg",
			argv[1]);
	config_fd = s_open(path, 0);
	if (config_fd < 0) {
		perror("Can't open global.cfg");
		exit(-1);
	}
	s_dup2(config_fd, 0);
	while (fgets(prop_string, NAME_MAX - 1, stdin) != NULL) {
		for (int i = strlen(prop_string) - 1; i >= 0 && (prop_string[i] == ' ' || prop_string[i] == '\n'); --i)
			prop_string[i] = 0;
		if (!strncmp(prop_string, "score_parameter =", 17)) {
			if (strstr(prop_string, "SUMM") != NULL)
				score_parameter = SUMM;
			if (strstr(prop_string, "PERF") != NULL)
				score_parameter = PERF;
		}
		if (!strncmp(prop_string, "problems_list =", 15)) {
			int tmp = 15;
			if (prop_string[16] == ' ')
				tmp = 16;
			for (tmp = 15; tmp < strlen(prop_string) && prop_string[tmp] == ' '; ++tmp)
				;
			strncpy(problems_string, prop_string + tmp, NAME_MAX);
		}
	}
	s_close(0);
}

char **get_problems(char *string)
{
	char **list = malloc(strlen(string) * sizeof(char **));
	int start = 0, j = 0;
	for (int i = 0; i <= strlen(string); ++i) {
		if (string[i] == ',' || string[i] == 0) {
			list[j] = malloc((i - start + 1) * sizeof(char));
			strncpy(list[j], string + start, i - start);
			list[j++][i - start] = 0;
			start = i + 2;
		}
	}
	list[j] = NULL;
	return list;
}

void free_problems(char **list)
{
	for (int i = 0; list[i] != NULL; ++i)
		free(list[i]);
	free(list);
}

int printResult(Participant *prt)
{
	char **problems_list = get_problems(problems_string);
	char flag;

	printf("Name, %s\n", problems_string);

	for (int i = 0; prt[i].name != NULL; ++i) {
		printf("%s", prt[i].name);
		for (int j = 0; problems_list[j] != NULL; ++j) {
			flag = 1;
			for (int k = 0; prt[i].solutions[k] != 0; ++k) {
				if (strlen(problems_list[j]) &&
						strncmp(problems_list[j], prt[i].solutions[k],
							strlen(problems_list[j])) == 0) {
					printf(", %d", prt[i].points[k]);
					flag = 0;
					break;
				}
			}
			if (flag)
				printf(", 0");
		}
		puts("");
	}
	free_problems(problems_list);
	return 0;
}

void freeParticipants(Participant *prt)
{
	for (int i = 0; prt[i].name != NULL; ++i) {
		free(prt[i].solutions);
		free(prt[i].points);
	}
	free(prt);
}

int main(int argc, char **argv)
{
	Participant *participants;

	init(argc, argv);
	participants = get_participants_list(argv[1]);
	testing(participants, argv[1]);
	printResult(participants);
	freeParticipants(participants);
	return 0;
}
