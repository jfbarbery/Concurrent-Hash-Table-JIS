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
#include <sys/time.h>
#include <inttypes.h>

#define MAX_COMMAND_LENGTH 6
#define MAX_NAME_LENGTH 50

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

int setupFD();
hashRecord* createHashTable();
void* thread_function(void* arg);
void insert(char* key, uint32_t value);
void delete(char* key);
void search(char* key);
int get_num_threads_helper(char* thread_info);
int num_digits_after_first_digit(char* thread_info, int i);
int get_num_threads(int fd);
char* parse_until(int fd, char c);
char* parse_string_until(char* str, char c);
void print();
long long current_timestamp();

int debug = 0;
int eof = 0;
int total_locks = 0;
int total_unlocks = 0;
FILE* out = NULL;
// Global head of the linked list (dummy node)
// ** FIRST ITEM IN LIST IS A PLACEHOLDER, NOT AN ACTUAL ENTRY **
hashRecord* record = NULL;

// Global read-write lock
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;


int main(void)
{
	int fd = setupFD();
	if (fd != -1)
	{
		if (debug) printf("Successfully opened the file.\n");
	}
	else
	{
		if (debug) printf("Error opening file. Is commands.txt contained in this directory?\n");
		return 1;
	}
	out = fopen("output.txt", "w");
	// Initialize the record
	hashRecord* record = createHashTable();
	
	// Read the number of threads
	int num_threads = get_num_threads(fd);
	fprintf(out, "Running on %d threads\n", num_threads);
	
	// Allocate array of pthread_t
	pthread_t* thread_array = (pthread_t*) malloc(sizeof(pthread_t) * num_threads);
	// Allocate array of thread info
	threadRecord* thread_info_array = (threadRecord*) malloc(sizeof(threadRecord) * num_threads);

	// Check malloc success
	if (thread_array == NULL)
	{
		if (debug) printf("Failed to allocate thread array.\n");
		return 1;
	}
	
	if (debug) printf("Number of threads: %d\n\n", num_threads);
	
	// Process commands
	int command_to_parse = 1;
	int index = 0;
	char* command;
	do
	{
		threadRecord* temp = malloc(sizeof(threadRecord));
		temp->command = (char*) malloc(sizeof(char) * (MAX_COMMAND_LENGTH+1));
		temp->key = (char*) malloc(sizeof(char) * (MAX_NAME_LENGTH+1));
		
		// Parse command
		command = parse_until(fd, ',');
		if (eof)
		{
			free(command);
			free(temp->command);
			free(temp->key);
			free(temp);
			break; // We may be at EOF without being told it yet
		}
		// Store thread command info in the list
		strcpy(temp->command, command);
		if (debug) printf("Command: |%s|\n", command);
		// Save memory
		free(command);
		
		// Parse name
		char* name = parse_until(fd, ',');
		// Store name info in the list
		strcpy(temp->key, name);
		if (debug) printf("Name: |%s|\n", name);
		// Save memory
		free(name);
		
		// Parse salary
		char* salary_string = parse_until(fd, '\n');
		int salary = atoi(salary_string);
		// Store salary info in the list
		temp->value = salary;
		if (debug) printf("Salary: %d\n", salary);
		// Save memory
		free(salary_string);
		
		// Store all copied values into array
		thread_info_array[index] = *temp;
		
		if (debug) printf("\n");
		index++;
		
	} while (!eof);
	
	close(fd);
	if (debug) printf("Successfully closed the file.\n\n");
	// Create and execute all the insert threads
	for (int i = 0; i < num_threads; i++)
	{
		if (!strcmp(thread_info_array[i].command, "insert"))
		{
			pthread_create(&thread_array[i], NULL, thread_function, (void*)&thread_info_array[i]);
		}
	}
	// Join all insert threads here
	for (int i = 0; i < num_threads; i++)
	{
		if (!strcmp(thread_info_array[i].command, "insert"))
		{
			pthread_join(thread_array[i], NULL);
		}
	}
	
	// Create and execute all the non-insert threads (everything else)
	for (int i = 0; i < num_threads; i++)
	{
		if (strcmp(thread_info_array[i].command, "insert"))
		{
			pthread_create(&thread_array[i], NULL, thread_function, (void*)&thread_info_array[i]);
		}
	}
	// Join the rest of the threads (non insert) here
	for (int i = 0; i < num_threads; i++)
	{
		if (strcmp(thread_info_array[i].command, "insert"))
		{
			pthread_join(thread_array[i], NULL);
		}
	}
	fprintf(out, "Finished all threads.\n");
	fprintf(out, "Total unlocks: %d\n", total_unlocks);
	fprintf(out, "Total locks: %d\n", total_locks);
	
	print();
	
	free(thread_array);
	free(thread_info_array);
	free(record);
	fclose(out);
	return 0;
}

int setupFD()
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
	
	return open(filename, O_RDONLY);
}

hashRecord* createHashTable()
{
	record = (hashRecord*) malloc(sizeof(hashRecord));
	record->next = NULL;
	return record;
}

void* thread_function(void* arg)
{
	threadRecord* args = (threadRecord*) arg;
	if (!strcmp(args->command, "insert"))
	{
		insert(args->key, args->value);
	}
	else if (!strcmp(args->command, "delete"))
	{
		delete(args->key);
	}
	else if (!strcmp(args->command, "search"))
	{
		search(args->key);
	}
	else if (debug) printf("How'd you get here?\n");
	return NULL;
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
	uint8_t hash = jenkins_one_at_a_time_hash((uint8_t*) key, strlen(key));
	fprintf(out, "%lld: INSERT,%d,%s,%d\n", current_timestamp(), hash, key, value);
	pthread_rwlock_wrlock(&rwlock);
	total_locks++;
	fprintf(out, "%lld: WRITE LOCK ACQUIRED\n", current_timestamp());

	hashRecord* curr = record;
	while (curr->next != NULL)
	{
		if (strcmp(curr->next->name, key) == 0)
		{
			curr->next->salary = value;
			fprintf(out, "%lld: WRITE LOCK RELEASED\n", current_timestamp());
			total_unlocks++;
			pthread_rwlock_unlock(&rwlock);
			return;
		}
		curr = curr->next;
	}
	hashRecord* newNode = (hashRecord*) malloc(sizeof(hashRecord));
	newNode->hash = hash;
	strcpy(newNode->name, key);
	newNode->salary = value;
	newNode->next = NULL;
	curr->next = newNode;
	fprintf(out, "%lld: WRITE LOCK RELEASED\n", current_timestamp());
	total_unlocks++;
	pthread_rwlock_unlock(&rwlock);
	
}

// Delete by key
void delete(char* key)
{
	fprintf(out, "%lld: DELETE,%s\n", current_timestamp(), key);
	pthread_rwlock_rdlock(&rwlock);
	total_locks++;
	fprintf(out, "%lld: READ LOCK ACQUIRED\n", current_timestamp());

	hashRecord* prev = record;
	hashRecord* curr = record->next;
	while (curr != NULL)
	{
		if (strcmp(curr->name, key) == 0)
		{
			fprintf(out, "%lld: READ LOCK RELEASED\n", current_timestamp());
			total_unlocks++;
			pthread_rwlock_unlock(&rwlock);
			pthread_rwlock_wrlock(&rwlock);
			total_locks++;
			fprintf(out, "%lld: WRITE LOCK ACQUIRED\n", current_timestamp());
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
			fprintf(out, "%lld: WRITE LOCK RELEASED\n", current_timestamp());
			total_unlocks++;
			pthread_rwlock_unlock(&rwlock);
			return;
		}
		prev = curr;
		curr = curr->next;
	}
	fprintf(out, "%lld: WRITE LOCK RELEASED\n", current_timestamp());
	total_unlocks++;
	pthread_rwlock_unlock(&rwlock);
}

// Search and print result
void search(char* key)
{
	pthread_rwlock_rdlock(&rwlock);
	total_locks++;
	fprintf(out, "%lld: READ LOCK ACQUIRED\n", current_timestamp());

	hashRecord* curr = record->next;
	while (curr != NULL)
	{
		if (strcmp(curr->name, key) == 0)
		{
			fprintf(out, "%lld: SEARCH: FOUND %s\n", current_timestamp(), key);
			fprintf(out, "%lld: READ LOCK RELEASED\n", current_timestamp());
			total_unlocks++;
			pthread_rwlock_unlock(&rwlock);
			return;
		}
		curr = curr->next;
	}

	fprintf(out, "%lld: SEARCH: NOT FOUND\n", current_timestamp());
	fprintf(out, "%lld: READ LOCK RELEASED\n", current_timestamp());
	total_unlocks++;
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
	char* line_info = (char*) malloc(sizeof(char) * (MAX_NAME_LENGTH+1));
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
			eof = 1;
			break;
		}
		else if (ret_value <= (MAX_NAME_LENGTH+1))
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

void print()
{
	hashRecord* cur = record->next;
	while (cur != NULL)
	{
		fprintf(out, "%" PRIu32 ",%s,%d\n", cur->hash, cur->name, cur->salary);
		cur = cur->next;
	}
}

long long current_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  long long microseconds = (te.tv_sec * 1000000) + te.tv_usec; // calculate milliseconds
  return microseconds;
}

