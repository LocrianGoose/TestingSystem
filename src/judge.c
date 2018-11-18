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
	int points;
} Participant;


void s_chdir(char *path);
int s_open(char *fileName, int flag);
void *s_realloc(void *ptr, int size);

char **get_solutions(DIR *sol_dir)
{
	char **solutions = NULL;
	char *tmp;
	struct dirent *entry;
	int i = 0, max_len = 1;

	while ((entry = readdir(sol_dir)) != NULL) {
		if (i + 1 >= max_len) {
			max_len <<= 1;
			solutions = s_realloc(solutions,
					max_len * sizeof(char *));
		}
		tmp = entry->d_name;
		if (strncmp(tmp, ".", 2) && strncmp(tmp, "..", 3)) //add filter
			solutions[i++] = tmp;
	}
	if (i == 0)
		free(solutions);
	solutions[i] = NULL;
	return solutions;
}

Participant *get_participants_list(void)
{
	int i = 0, max_len = 1;
	struct dirent *entry;
	char *tmp;
	DIR *participants_dir = opendir("participants"), *sol_dir;
	Participant *participants = NULL;

	if (participants_dir == NULL) {
		perror("opendir failed");
		exit(1);
	}
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
				participants[i].solutions =
					get_solutions(sol_dir);
				i++;
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

void testing(Participant *prts, char *contest)
{
	pid_t pid;

	for (int i = 0; prts[i].name != NULL; i++) {
		for (int j = 0; prts[i].solutions[j] != NULL; j++) {
			pid = fork();
			if (pid < 0) {
				perror("fork failed");
			} else if (pid == 0) {
				//getresult
			} else {
				if (execlp("tester", "tester",
						contest, prts[i].name,
						prts[i].solutions[j],
						NULL) < 0) {
					perror("Tried to exec tester, but failed");
				}
			}
		}
	}
}


void init(int argc, char **argv, int *config)
{
	if (argc != 2) {
		perror("Wrong number of arguments");
		exit(1);
	}
	s_chdir(argv[1]);
	*config = s_open("global.cfg", 0);
	if (*config < 0) {
		perror("Can't open global.cfg");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int config;
	Participant *participants;

	init(argc, argv, &config);
	participants = get_participants_list();
	//testing(participants, argv[1]);
	free(participants);
	close(config);
	return 0;
}
