#include <stdio.h>

typedef struct hash_struct
{
	// Hash value produced by running the name text through the hash function
	uint32_t hash;
	// Full name of person
	char name[50];
	// Salary of person
	uint32_t salary;
	struct hash_struct *next;
} hashRecord;

int main(void)
{
	printf("Hello, World!\n");
	return 0;
}
