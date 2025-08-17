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
    if (dbhdr == NULL || employees == NULL || addstring == NULL || addstring[0] == '\0') {
        printf("Invalid NULL arguments.\n");
        return STATUS_ERROR;
    }

	if (dbhdr->count < 0 || dbhdr->count > MAX_EMPLOYEES) {
		printf("Invalid employee count: %d\n", dbhdr->count);
		return STATUS_ERROR;
	}

	int cur_cnt = dbhdr->count;
	(dbhdr->count)++;

	struct employee_t* tmp = realloc(*employees, sizeof(struct employee_t)*(dbhdr->count));
	if (tmp == NULL) {
		perror("realloc\n");
		return STATUS_ERROR;
	}
	*employees = tmp;

	memset(&((*employees)[cur_cnt]), 0, sizeof(struct employee_t));

	char tmpstr[MAX_STRING_LEN];
	strncpy(tmpstr, addstring, sizeof(tmpstr));
	tmpstr[sizeof(tmpstr)-1] = '\0';

	char *name = strtok(tmpstr, ",");
	char *address = strtok(NULL, ",");
	char *hours = strtok(NULL, ",");

	if (!name || !address || !hours) {
		perror("strtok malformed string.\n");
		(dbhdr->count)--;
		free(*employees);
		*employees = NULL;
		return STATUS_ERROR;
	}

	strncpy((*employees)[dbhdr->count-1].name, name, sizeof((*employees)[dbhdr->count-1].name));
	strncpy((*employees)[dbhdr->count-1].address, address, sizeof((*employees)[dbhdr->count-1].address));
	//snprintf((*employees)[cur_cnt].name, sizeof((*employees)[cur_cnt].name), "%s", name);
	//snprintf((*employees)[cur_cnt].address, sizeof((*employees)[cur_cnt].address), "%s", address);
	(*employees)[cur_cnt].hours = atoi(hours);

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

	if (dbhdr->count < 0 || dbhdr->count > MAX_EMPLOYEES) {
		printf("Invalid employee count: %d\n", dbhdr->count);
		return STATUS_ERROR;
	}

	if (dbhdr->count == 0) {
		printf("No employees to read.\n");
		*employeesOut = NULL;
		return STATUS_SUCCESS;
	}

	int count = dbhdr->count;
	ssize_t n = 0;

	struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if (employees == NULL) {
		printf("Malloc failed\n");
		return STATUS_ERROR;
	}

	n = read(fd, employees, count*sizeof(struct employee_t));
	if (n != count * sizeof(struct employee_t)) {
		perror("Failed to read employees\n");
		free(employees);
		return STATUS_ERROR;
	}

	int i = 0;
	for (; i < count; i++) {
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

	if (dbhdr == NULL) {
		printf("Invalid NULL arguments.\n");
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

	if (employees == NULL) {
		printf("No employees to write.\n");
		return STATUS_SUCCESS;
	}

	for (int i = 0; i < real_cnt; i++) {
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
	if (headerOut == NULL) {
		printf("headerOut is empty.\n");
		return STATUS_ERROR;
	}

	dbheader_t *header = calloc(1, sizeof(dbheader_t));
	if (header == NULL) {
		printf("Malloc failed to create db header.\n");
		return STATUS_ERROR;
	}
	header->version = 0x1;
	header->count = 0;
	header->magic = HEADER_MAGIC;
	header->filesize = sizeof(struct dbheader_t);

	*headerOut = header;
	return STATUS_SUCCESS;
}


