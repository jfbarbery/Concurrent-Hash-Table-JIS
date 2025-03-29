#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINE_LENGTH 50

typedef struct hash_struct
{
	// Hash value produced by running the name text through the hash function
	uint32_t hash;
	// Full name of person (key)
	char name[50];
	// Salary of person (value)
	uint32_t salary;
	struct hash_struct *next;
} hashRecord;

void insert(char* key, uint32_t value);
void delete(char* key);
void search(char* key);
int get_num_threads_helper(char* thread_info);
int num_digits_after_first_digit(char* thread_info, int i);
int get_num_threads(int fd);
char* parse_until(int fd, char c);
char* parse_string_until(char* str, char c);

const int debug = 1;

int main(void)
{
	int fd = open("commands.txt", O_RDONLY);
	if (fd != -1)
	{
		if (debug) printf("Successfully opened the file.\n");
	}
	else
	{
		if (debug) printf("Error opening file. Is commands.txt contained in this directory?\n");
		return 1;
	}
	
	// Read the number of threads
	int num_threads = get_num_threads(fd);
	if (debug) printf("Number of threads: %d\n\n", num_threads);
	
	// Process commands
	int command_to_parse = 1;
	char* command;
	do
	{
		// Parse command
		command = parse_until(fd, ',');
		if (!strcmp(command, "insert"))
		{
			if (debug) printf("insert command\n");
		}
		else if (!strcmp(command, "delete"))
		{
			if (debug) printf("delete command\n");
		}
		else if (!strcmp(command, "search"))
		{
			if (debug) printf("search command\n");
		}
		else
		{
			if (debug) printf("Unrecognized command.\n");
			command_to_parse = 0;
			//return -1; // Optionally end program here
		}
		free(command);
		
		// Parse name
		char* name = parse_until(fd, ',');
		if (debug) printf("Name: |%s|\n", name);
		free(name);
		
		// Parse salary
		char* salary_string = parse_until(fd, '\n');
		if (!strcmp(command, "insert"))
		{
			int salary = atoi(salary_string);
			if (debug) printf("Salary: %d\n", salary);
		}
		free(salary_string);
		if (debug) printf("\n");
	} while (command_to_parse);
	
	close(fd);
	if (debug) printf("Successfully closed the file.\n");
	
	return 0;
}

void insert(char* key, uint32_t value)
{
	
}

void delete(char* key)
{
	
}

void search(char* key)
{
	
}

int get_num_threads_helper(char* thread_info)
{
	// Parse thread info for number of threads
	for (int i = 0; thread_info[i] != '\0'; i++)
	{
		// Once we find the first digit, use helper to find how many digits follow
		if (isdigit(thread_info[i]))
		{
			int num_digits = num_digits_after_first_digit(thread_info, i) + 1;
			char* num_threads_string = (char*) malloc(sizeof(char) * (num_digits + 1));
			// Copy num characters from thread_info into num_threads_string
			// In other words, copy just the number of threads as a string
			for (int j = 0; j < num_digits; j++)
			{
				num_threads_string[j] = thread_info[i+j];
			}
			int num_threads = atoi(num_threads_string);
			free(num_threads_string);
			return num_threads;
		}
	}
	return -1;
}

int num_digits_after_first_digit(char* thread_info, int i)
{
	int ret = 0;
	for (i += 1; thread_info[i] != '\0'; i++)
	{
		if (isdigit(thread_info[i]))
		{
			ret++;
		}
		else break;
	}
	return ret;
}

int get_num_threads(int fd)
{
	int num_threads = 0;
	char* thread_info = parse_until(fd, '\n');
	num_threads = get_num_threads_helper(thread_info);
	free(thread_info);
	
	return num_threads;
}
// Reads the contents from the file descriptor until c is read.
// The next character to be read will be on the following line.
char* parse_until(int fd, char c)
{
	char* line_info = (char*) malloc(sizeof(char) * MAX_LINE_LENGTH);
	char* buf = (char*) malloc(sizeof(char));
	
	int i = 0;
	line_info[0] = '\0';
	int c_not_found = 1;
	while (c_not_found)
	{
		ssize_t ret_value = read(fd, buf, 1);
		if (buf[0] == c)
		{
			c_not_found = 0;
			break;
		}
		if (ret_value == 0)
		{
			if (debug) printf("Reached end of file.\n");
			break;
		}
		else if (ret_value <= MAX_LINE_LENGTH)
		{
			line_info[i] = buf[0];
		}
		i++;
	}
	free(buf);
	line_info[i+1] = '\0';
	return line_info;
}

// Parse str until c is found, return the substring parsed.
char* parse_string_until(char* str, char c)
{
	int len = 0;
	int i = 0;
	for (; str[i] != c; i++)
	{
		i++;
	}
	len = i;
	char* parsed_string = (char*) malloc(sizeof(char) * (len + 1));
	for (i = 0; i < len; i++)
	{
		parsed_string[i] = str[i];
	}
	return parsed_string;
}


