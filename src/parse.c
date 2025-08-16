#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
	for (int i = 0; i < dbhdr->count; i++) {
		printf("Employee %d\n", i);
		printf("\tName: %s\n", employees[i].name);
		printf("\tAddress: %s\n", employees[i].address);
		printf("\tHours: %d\n", employees[i].hours);
	}
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
	char *name = strtok(addstring, ",");
	char *address = strtok(NULL, ",");
	char *hours = strtok(NULL, ",");

	strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
	strncpy(employees[dbhdr->count-1].address, address, sizeof(employees[dbhdr->count-1].address));
	employees[dbhdr->count-1].hours = atoi(hours);
	printf("%s: %s: %s\n", name, address, hours);
	return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
	if (fd < 0) {
		printf("Bad FD from the user\n");
		return STATUS_ERROR;
	}

	int count = dbhdr->count;
	employee_t *employees = calloc(count, sizeof(employee_t));

	if (employees == NULL) {
		printf("Malloc failed.\n");
		return STATUS_ERROR;
	}

	if ((read(fd, employees, count * sizeof(employee_t))) < 0) {
		printf("Read failed.\n");
		return STATUS_ERROR;
	}

	for (int i = 0; i < count; i++) {
		employees[i].hours = ntohl(employees[i].hours);
	}
	*employeesOut = employees;
	return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
	if (fd < 0) {
		printf("Bad FD from the user\n");
		return STATUS_ERROR;
	}

	dbheader_t out_header = {0};
	out_header.magic = htonl(dbhdr->magic);
	out_header.version = htons(dbhdr->version);
	out_header.filesize = htonl(sizeof(dbheader_t) + (dbhdr->count * sizeof(employee_t)));
	out_header.count = htons(dbhdr->count);

	lseek(fd, 0, SEEK_SET);
	if ((write(fd, &out_header, sizeof(dbheader_t))) == -1) {
		perror("Failed to write db header into file.\n");
		return STATUS_ERROR;
	}

	for (int i = 0; i < dbhdr->count; i++) {
		employees[i].hours = htonl(employees[i].hours);
		if ((write(fd, &employees[i], sizeof(employee_t))) == -1) {
			perror("Failed to write employee info into file.\n");
			return STATUS_ERROR;
		}
	}

	return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
	if (fd < 0) {
		printf("Bad FD from the user\n");
		return STATUS_ERROR;
	}

	dbheader_t *header = calloc(1, sizeof(dbheader_t));
	if (header == NULL) {
		printf("Malloc failed to create a db header.\n");
		return STATUS_ERROR;
	}

	if (read(fd, header, sizeof(dbheader_t)) != sizeof(dbheader_t)) {
		perror("Read");
		free(header);
		return STATUS_ERROR;
	}

	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->magic = ntohl(header->magic);
	header->filesize = ntohl(header->filesize);

	if (header->version != 1) {
		printf("Improper header version.\n");
		free(header);
		return STATUS_ERROR;
	}
	if (header->magic != HEADER_MAGIC) {
		printf("Improper header magic.\n");
		free(header);
		return STATUS_ERROR;
	}

	struct stat dbstat = {0};
	fstat(fd, &dbstat);
	if (header->filesize != dbstat.st_size) {
		printf("Corrupted database.\n");
		free(header);
		return STATUS_ERROR;
	}

	*headerOut = header;
	return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t **headerOut) {
	dbheader_t *header = calloc(1, sizeof(dbheader_t));	
	
	if (header == NULL) {
		printf("Malloc failed to create db header.\n");
		return STATUS_ERROR;
	}
	header->version = 0x1;
	header->count = 0x0;
	header->magic = HEADER_MAGIC;
	header->filesize = sizeof(dbheader_t);

	*headerOut = header;
	return STATUS_SUCCESS;
}


