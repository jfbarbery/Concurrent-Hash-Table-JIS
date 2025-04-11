#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

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

typedef struct thread_info
{
	// Key
	char* key;
	// Value (if applicable to command)
	uint32_t value;
	// Function to call
	char* command;
} threadRecord;

hashRecord* createHashTable();
void insert(char* key, uint32_t value);
void delete(char* key);
void search(char* key);
int get_num_threads_helper(char* thread_info);
int num_digits_after_first_digit(char* thread_info, int i);
int get_num_threads(int fd);
char* parse_until(int fd, char c);
char* parse_string_until(char* str, char c);

const int debug = 1;
// Global head of the linked list (dummy node)
hashRecord* record = NULL;

// Global read-write lock
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;


int main(void)
{
	// Check to see if "commands.txt" is contained in this directory
	// (Our source code does not contain commands.txt, so if this is
	// not present, it means use our own commands.txt file in dev/)
	DIR* dirp = opendir("./");
	if (dirp != NULL)
	{
		if (debug) printf("Successfully opened the directory.\n");
	}
	else
	{
		if (debug) printf("Error opening directory. What...\n");
		return -1;
	}
	struct dirent* entry = readdir(dirp);
	char* filename = "";
	while (entry != NULL)
	{
		// If "commands.txt" is contained in this directory
		if (!strcmp(entry->d_name, "commands.txt"))
		{
			filename = "commands.txt";
			break;
		}
		entry = readdir(dirp);
	}
	closedir(dirp);
	// If "commands.txt" was not contained in this directory
	if (!strcmp(filename, ""))
	{
		// Testing file
		filename = "dev/commands.txt";
	}
	
	// Open commands file
	int fd = open(filename, O_RDONLY);
	if (fd != -1)
	{
		if (debug) printf("Successfully opened the file.\n");
	}
	else
	{
		if (debug) printf("Error opening file. Is commands.txt contained in this directory?\n");
		return 1;
	}
	// Initialize the record
	hashRecord* record = createHashTable();
	
	// Read the number of threads
	int num_threads = get_num_threads(fd);
	
	// Allocate array of pthread_t
	pthread_t* thread_array = (pthread_t*) malloc(sizeof(pthread_t) * num_threads);
	// Allocate array of thread info
	threadRecord* thread_info_array = (threadRecord*) malloc(sizeof(threadRecord) * num_threads);

	// Check malloc success
	if (thread_array == NULL)
	{
		fprintf(stderr, "Failed to allocate thread array.\n");
		exit(EXIT_FAILURE);
	}
	
	if (debug) printf("Number of threads: %d\n\n", num_threads);
	
	// Process commands
	int command_to_parse = 1;
	int index = 0;
	char* command;
	do
	{
		// Parse command
		command = parse_until(fd, ',');
		// Store thread insert info in the list
		strcpy(thread_info_array[i]->command, command);
		// Save memory
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
	
	// Create and execute all the insert threads
	for (int i = 0; i < num_threads; i++)
	{
		if (!strcmp(thread_info_array[i]->command, "insert"))
		{
			thread_array[i] = pthread_create(&thread_array[i], NULL, thread_function, arg);
		}
	}

	free(thread_array);
	free(thread_info_array);
	free(record);
	
	return 0;
}

hashRecord* createHashTable()
{
	record = (hashRecord*) malloc(sizeof(hashRecord));
	record->next = NULL;
	return record;
}

// Jenkins one-at-a-time hash function
uint32_t jenkins_one_at_a_time_hash(const uint8_t* key, size_t length)
{
	size_t i = 0;
	uint32_t hash = 0;
	while (i != length)
	{
		hash += key[i++];
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	return hash;
}

// Insert or update
void insert(char* key, uint32_t value)
{
	pthread_rwlock_wrlock(&rwlock);

	hashRecord* curr = record;
	while (curr->next != NULL)
	{
		if (strcmp(curr->next->name, key) == 0)
		{
			curr->next->salary = value;
			pthread_rwlock_unlock(&rwlock);
			return;
		}
		curr = curr->next;
	}

	hashRecord* newNode = (hashRecord*) malloc(sizeof(hashRecord));
	newNode->hash = jenkins_one_at_a_time_hash((uint8_t*) key, strlen(key));
	strcpy(newNode->name, key);
	newNode->salary = value;
	newNode->next = record->next;
	record->next = newNode;

	pthread_rwlock_unlock(&rwlock);
}

// Delete by key
void delete(char* key)
{
	pthread_rwlock_rdlock(&rwlock);

	hashRecord* prev = record;
	hashRecord* curr = record->next;
	while (curr != NULL)
	{
		if (strcmp(curr->name, key) == 0)
		{
			pthread_rwlock_unlock(&rwlock);

			pthread_rwlock_wrlock(&rwlock);
			prev = record;
			curr = record->next;
			while (curr != NULL)
			{
				if (strcmp(curr->name, key) == 0)
				{
					prev->next = curr->next;
					free(curr);
					break;
				}
				prev = curr;
				curr = curr->next;
			}
			pthread_rwlock_unlock(&rwlock);
			return;
		}
		prev = curr;
		curr = curr->next;
	}

	pthread_rwlock_unlock(&rwlock);
}

// Search and print result
void search(char* key)
{
	pthread_rwlock_rdlock(&rwlock);

	hashRecord* curr = record->next;
	while (curr != NULL)
	{
		if (strcmp(curr->name, key) == 0)
		{
			printf("Found: %s with salary %u\n", curr->name, curr->salary);
			pthread_rwlock_unlock(&rwlock);
			return;
		}
		curr = curr->next;
	}

	printf("Key not found: %s\n", key);
	pthread_rwlock_unlock(&rwlock);
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
// The next character to be read will be the character following c.
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


