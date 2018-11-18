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
	DIR *dir;
	char *name;
	int points;
} Participant;

void s_chdir(char *path);
int s_open(char *fileName, int flag);
void *s_realloc(void *ptr, int size);

void compile(char *name)
{
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork failed");
		exit(-1);
	} else if (pid) {
		wait(NULL);
	} else {
		execlp("make", "make", name, NULL);
	}
}

Participant *get_participants_list(void)
{
	int i = 0, max_len = 1;
	struct dirent *entry;
	char *tmp;
	DIR *participants_dir = opendir("participants");
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
		participants[i].dir = opendir(entry->d_name);
		if (participants[i].dir != NULL) {
			participants[i].name = (tmp = entry->d_name);
			participants[i].points = 0;
			if (strncmp(tmp, ".", 2) && strncmp(tmp, "..", 3)) {
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
	free(participants);
	close(config);
	return 0;
}
