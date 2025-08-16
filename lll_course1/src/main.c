#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
	printf("Usage: %s -n -f <database file>\n", argv[0]);
	printf("\t -n	- create new database file\n");
	printf("\t -n	- (required) path to database file\n");
}

int main(int argc, char *argv[]) { 
	int op = 0;
	char *filepath = NULL, *addstring = NULL;
	bool newfile = false, list = false;
	int dbfd = -1;
	dbheader_t *header = NULL;
	employee_t *employees = NULL;

	while ((op = getopt(argc, argv, "nf:a:l")) != -1) {
		switch(op) {
			case 'f':
				filepath = optarg;
				//printf("This is option f\n");
				break;
			case 'n':
				newfile = true;
				//printf("This is option n\n");
				break;
			case 'a':
				addstring = optarg;
				break;
			case 'l':
				list = true;
				break;
			case '?':
				//printf("Unknown option -%c\n", op);
				break;
			default:
				return -1;
		}
	}

	if (filepath == NULL) {
		printf("Filepath is a required argument\n");
		print_usage(argv);
		return 0;
	}

	if (newfile) {
		if ((dbfd = create_db_file(filepath)) == STATUS_ERROR) {
			printf("Unable to create database file.\n");
			return -1;
		}
		
		if (create_db_header(dbfd, &header) == STATUS_ERROR) {
			printf("Failed to create database header.\n");
			close(dbfd);
			return -1;
		}
	} else {
		dbfd = open_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			printf("Unable to open database file.\n");
			close(dbfd);
			return -1;
		}
		if ((validate_db_header(dbfd, &header)) == STATUS_ERROR) {
			printf("Failed to validate database header.\n");
			close(dbfd);
			return -1;
		}

	}
	printf("Newfile: %d\n", newfile);
	printf("filepath: %s\n", filepath);

	if (read_employees(dbfd, header, &employees) == STATUS_ERROR) {
		printf("Failed to read employees.\n");
		close(dbfd);
		return 0;
	}

	if (addstring) {
		header->count++;
		employees = realloc(employees, sizeof(employee_t) * header->count);
		add_employee(header, employees, addstring);
	}

	if (list) {
		list_employees(header, employees);
	}

	if (output_file(dbfd, header, employees) == STATUS_ERROR) {
		printf("Failed to output database header.\n");
		close(dbfd);
		return -1;
	}
	close(dbfd);
	return 0;
}
