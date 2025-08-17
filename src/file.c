#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file.h"
#include "common.h"


int create_db_file(char *filename) {
	if (filename == NULL) {
		printf("Filename is NULL.\n");
		return STATUS_ERROR;
	}

	int dbfd = open(filename, O_RDONLY);
	if (dbfd != -1) {
		close(dbfd);
		printf("File already exists.\n");
		return STATUS_ERROR;
	}

	dbfd = open(filename, O_RDWR | O_CREAT, 0644);
	if (dbfd == -1) {
		perror("open");
		return STATUS_ERROR;
	}
	return dbfd;
}

int open_db_file(char *filename) {
	if (filename == NULL) {
		printf("Filename is NULL.\n");
		return STATUS_ERROR;
	}

	int dbfd = open(filename, O_RDWR);
	if (dbfd == -1) {
		printf("Failed to open database file: %s.\n", filename);
		return STATUS_ERROR;
	}
	return dbfd;
}
