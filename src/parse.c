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
	if (dbhdr == NULL || employees == NULL) {
		printf("Invalid NULL arguments.\n");
		return;
	}

	for (int i = 0; i < dbhdr->count; i++) {
		printf("Employee %d\n", i);
		printf("\tName: %s\n", employees[i].name);
		printf("\tAddress: %s\n", employees[i].address);
		printf("\tHours: %d\n", employees[i].hours);
	}
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring) {
	if (dbhdr == NULL || addstring == NULL) {
		printf("Invalid NULL arguments.\n");
		return STATUS_ERROR;
	}

	printf("== Add employee, add string: %s, count %d===\n", addstring, dbhdr->count);
	struct employee_t* tmp= realloc(*employees, sizeof(struct employee_t)*(dbhdr->count));
	if (tmp == NULL) {
		perror("realloc\n");
		return STATUS_ERROR;
	}
	*employees = tmp;

	char *name = strtok(addstring, ",");
	char *address = strtok(NULL, ",");
	char *hours = strtok(NULL, ",");
	int cur_cnt = (dbhdr->count) - 1;

	if (!name || !address || !hours) {
   		perror("Invalid input\n"); 
    	return STATUS_ERROR;
	}

	printf("BEFORE COPY - %s: %s: %s\n", name, address, hours);

	snprintf((*employees)[cur_cnt].name, sizeof((*employees)[cur_cnt].name), "%s", name);
	snprintf((*employees)[cur_cnt].address, sizeof((*employees)[cur_cnt].address), "%s", address);
	(*employees)[cur_cnt].hours = atoi(hours);
	for (int i = 0; i < dbhdr->count; i++) {
		printf("%d - AFTER COPY: %s: %s: %d\n", i, (*employees)[i].name, (*employees)[i].address, (*employees)[i].hours);
	}
	printf("==========\n");

	return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
	if (fd < 0) {
		printf("Bad FD from the user\n");
		return STATUS_ERROR;
	}

	if (dbhdr == NULL) {
		printf("Invalid NULL arguments.\n");
		return STATUS_ERROR;
	}

	int count = dbhdr->count;
	printf("count: %d\n", dbhdr->count);
	*employeesOut = calloc(count, sizeof(struct employee_t));

	if (*employeesOut == NULL) {
	    printf("Malloc failed.\n");
	    return STATUS_ERROR;
	}
	
	// Read data directly into employeesOut
	if ((read(fd, *employeesOut, count * sizeof(struct employee_t))) < 0) {
	    printf("Read failed.\n");
	    return STATUS_ERROR;
	}

	for (int i = 0; i < count; i++) {
		(*employeesOut)[i].hours = ntohl((*employeesOut)[i].hours);
	}
	printf("DEBUG - read done.\n");
	
	return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
	if (fd < 0) {
		printf("Bad FD from the user\n");
		return STATUS_ERROR;
	}

	int real_cnt = dbhdr->count;

	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->version = htons(dbhdr->version);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (real_cnt * sizeof(struct employee_t)));
	dbhdr->count = htons(dbhdr->count);

	lseek(fd, 0, SEEK_SET);
	if ((write(fd, dbhdr, sizeof(struct dbheader_t))) == -1) {
		perror("Failed to write db header into file.\n");
		return STATUS_ERROR;
	}

	printf("DEBUG - output_file, count: %d\n", real_cnt);
	for (int i = 0; i < real_cnt; i++) {
		printf("---- %s: %s: %d -----\n", employees[i].name, employees[i].address, employees[i].hours);
		employees[i].hours = htonl(employees[i].hours);
		if ((write(fd, &employees[i], sizeof(struct employee_t))) == -1) {
			perror("Failed to write employee info into file.\n");
			return STATUS_ERROR;
		}
	}

	return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
	struct stat dbstat = {0};

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

	if (headerOut == NULL) {
		printf("headerOut is empty.\n");
		return STATUS_ERROR;
	}
	
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


