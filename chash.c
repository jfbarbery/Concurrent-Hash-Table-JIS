#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

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

int main(void)
{
	printf("Hello, World!\n");
	
	int fd = open("commands.txt", O_RDONLY);
	printf("Successfully opened the file.\n");
	
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