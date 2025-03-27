#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

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
int num_digits_in_thread_info(char* thread_info, int len);
int num_digits_after_first_digit(char* thread_info, int i, int len);

int main(void)
{
	int fd = open("commands.txt", O_RDONLY);
	if (fd != -1)
	{
		printf("Successfully opened the file.\n");
	}
	else
	{
		printf("Error opening file. Is commands.txt contained in this directory?\n");
		return 1;
	}
	
	// Read the number of threads
	char* thread_info = (char*) malloc(sizeof(char) * MAX_LINE_LENGTH);
	char* buf = (char*) malloc(sizeof(char));
	int num_threads = 0;
	int i = 0;
	thread_info[0] = '\0';
	int flag = 1;
	while (flag)
	{
		ssize_t ret_value = read(fd, buf, 1);
		if (buf[0] == '\n')
		{
			flag = 0;
			break;
		}
		if (ret_value == 0)
		{
			printf("Reached end of file.\n");
		}
		else if (ret_value <= MAX_LINE_LENGTH)
		{
			thread_info[i] = buf[0];
		}
		i++;
	}
	free(buf);
	num_threads = num_digits_in_thread_info(thread_info, i);
	free(thread_info);
	//printf("%d\n", num_threads);
	
	// TODO: Process commands
	
	close(fd);
	printf("Successfully closed the file.\n");
	
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

int num_digits_in_thread_info(char* thread_info, int len)
{
	int num = 0;
	// Parse thread info for number of threads
	for (int i = 0; i < len; i++)
	{
		if (isdigit(thread_info[i]))
		{
			num = num_digits_after_first_digit(thread_info, i, len) + 1;
			break;
		}
	}
	return num;
}

int num_digits_after_first_digit(char* thread_info, int i, int len)
{
	int ret = 0;
	for (i += 1; i < len; i++)
	{
		if (isdigit(thread_info[i]))
		{
			ret++;
		}
		else break;
	}
	return ret;
}

