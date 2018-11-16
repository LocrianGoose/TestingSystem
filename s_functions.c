// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


int s_close(int fd)
{
	if (fd != 1 && fd != 0 && fd != -1 && fd != 2)
		if (close(fd)) {
			perror("close failed");
			return -1;
		}
	return 0;
}

void *s_realloc(void *ptr, int size)
{
	ptr = realloc(ptr, size);
	if (ptr == NULL) {
		perror("realloc failed");
		exit(1);
	}
	return ptr;
}

int s_open(char *fileName, int flag)
{
	int fd = -1;

	if (fileName == NULL) {
		return flag;
	} else if (flag == 0) {
		fd = open(fileName, O_RDONLY);
	} else if (flag == 1) {
		fd = open(fileName,
			O_RDWR | O_CREAT | O_TRUNC, 0666);
	}
	if (fd == -1) {
		perror("open failed");
		return -1;
	}
	return fd;
}

int s_dup2(int oldfd, int newfd)
{
	if (dup2(oldfd, newfd) == -1) {
		perror("dup2 failed");
		return -1;
	}
	s_close(oldfd);
	return 0;
}
