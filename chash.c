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
#include <time.h>

#define MAX_LINE_LENGTH 50

typedef struct hash_struct
{
	uint32_t hash;
	char name[50];
	uint32_t salary;
	struct hash_struct *next;
} hashRecord;

typedef struct
{
	char *command;
	char *name;
	char *param;
	FILE *out;
} ThreadArgs;

hashRecord *root = NULL;
hashRecord *tail = NULL;

// Threading variables
pthread_rwlock_t rwlock;
pthread_mutex_t insert_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t inserts_done = PTHREAD_COND_INITIALIZER;
int insert_count = 0;
int total_inserts = 0;
int lock_acquire_count = 0;
int lock_release_count = 0;

uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, size_t length)
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

void insert(char *name, uint32_t salary, FILE *out)
{
	uint32_t hash = jenkins_one_at_a_time_hash((const uint8_t *)name, strlen(name));
	time_t now = time(NULL);
	hashRecord *cur = root;

	while (cur != NULL)
	{
		if (cur->hash == hash && strcmp(cur->name, name) == 0)
		{
			cur->salary = salary;
			fprintf(out, "%ld,INSERT,%s,%u\n", now, name, salary);
			return;
		}
		cur = cur->next;
	}

	hashRecord *newNode = (hashRecord *)malloc(sizeof(hashRecord));
	newNode->hash = hash;
	strcpy(newNode->name, name);
	newNode->salary = salary;
	newNode->next = NULL;

	if (root == NULL)
	{
		root = newNode;
		tail = newNode;
	}
	else
	{
		tail->next = newNode;
		tail = newNode;
	}

	fprintf(out, "%ld,INSERT,%s,%u\n", now, name, salary);
}

void delete_record(char *name, FILE *out)
{
	time_t now = time(NULL);
	uint32_t target_hash = jenkins_one_at_a_time_hash((const uint8_t *)name, strlen(name));
	hashRecord *cur = root;

	if (cur == NULL)
		return;

	if (cur->hash == target_hash && strcmp(cur->name, name) == 0)
	{
		root = cur->next;
		free(cur);
		fprintf(out, "%ld,DELETE,%s\n", now, name);
		return;
	}

	while (cur->next != NULL)
	{
		if (cur->next->hash == target_hash && strcmp(cur->next->name, name) == 0)
		{
			hashRecord *temp = cur->next;
			cur->next = cur->next->next;
			free(temp);
			fprintf(out, "%ld,DELETE,%s\n", now, name);
			return;
		}
		cur = cur->next;
	}
}

void search(char *name, FILE *out)
{
	time_t now = time(NULL);
	uint32_t target_hash = jenkins_one_at_a_time_hash((const uint8_t *)name, strlen(name));
	hashRecord *cur = root;

	while (cur != NULL)
	{
		if (cur->hash == target_hash && strcmp(cur->name, name) == 0)
		{
			fprintf(out, "%ld,SEARCH,%s\n%u,%s,%u\n", now, name, cur->hash, cur->name, cur->salary);
			return;
		}
		cur = cur->next;
	}

	fprintf(out, "%ld,SEARCH,%s\nNo Record Found\n", now, name);
}

void print_all(FILE *out)
{
	int count = 0;
	for (hashRecord *temp = root; temp; temp = temp->next)
		count++;

	hashRecord **list = malloc(count * sizeof(hashRecord *));
	int i = 0;
	for (hashRecord *temp = root; temp; temp = temp->next)
		list[i++] = temp;

	for (int i = 0; i < count - 1; i++)
	{
		for (int j = i + 1; j < count; j++)
		{
			if (list[i]->hash > list[j]->hash)
			{
				hashRecord *tmp = list[i];
				list[i] = list[j];
				list[j] = tmp;
			}
		}
	}

	fprintf(out, "\n");
	for (int i = 0; i < count; i++)
	{
		fprintf(out, "%u,%s,%u\n", list[i]->hash, list[i]->name, list[i]->salary);
	}

	free(list);
}

char *parse_until(int fd, char delimiter)
{
	char *buffer = malloc(MAX_LINE_LENGTH);
	char ch;
	int idx = 0;
	while (read(fd, &ch, 1) == 1 && ch != delimiter && ch != '\n')
	{
		buffer[idx++] = ch;
	}
	buffer[idx] = '\0';
	return buffer;
}

int get_num_threads(int fd)
{
	char *info = parse_until(fd, '\n');
	int num = atoi(info + 8); // skip "threads,"
	free(info);
	return num;
}

void *handle_command(void *arg_ptr)
{
	ThreadArgs *args = (ThreadArgs *)arg_ptr;
	FILE *out = args->out;
	time_t now = time(NULL);

	if (strcmp(args->command, "insert") == 0)
	{
		pthread_rwlock_wrlock(&rwlock);
		lock_acquire_count++;
		fprintf(out, "%ld,WRITE LOCK ACQUIRED\n", now);
		insert(args->name, atoi(args->param), out);
		fprintf(out, "%ld,WRITE LOCK RELEASED\n", time(NULL));
		lock_release_count++;
		pthread_rwlock_unlock(&rwlock);

		pthread_mutex_lock(&insert_mutex);
		insert_count++;
		if (insert_count == total_inserts)
		{
			pthread_cond_broadcast(&inserts_done);
		}
		pthread_mutex_unlock(&insert_mutex);
	}
	else if (strcmp(args->command, "delete") == 0)
	{
		pthread_mutex_lock(&insert_mutex);
		while (insert_count < total_inserts)
		{
			fprintf(out, "%ld: WAITING ON INSERTS\n", now);
			pthread_cond_wait(&inserts_done, &insert_mutex);
		}
		pthread_mutex_unlock(&insert_mutex);

		pthread_rwlock_wrlock(&rwlock);
		lock_acquire_count++;
		fprintf(out, "%ld,WRITE LOCK ACQUIRED\n", time(NULL));
		delete_record(args->name, out);
		fprintf(out, "%ld,WRITE LOCK RELEASED\n", time(NULL));
		lock_release_count++;
		pthread_rwlock_unlock(&rwlock);
	}
	else if (strcmp(args->command, "search") == 0)
	{
		pthread_rwlock_rdlock(&rwlock);
		lock_acquire_count++;
		fprintf(out, "%ld,READ LOCK ACQUIRED\n", now);
		search(args->name, out);
		fprintf(out, "%ld,READ LOCK RELEASED\n", time(NULL));
		lock_release_count++;
		pthread_rwlock_unlock(&rwlock);
	}

	free(args->command);
	free(args->name);
	free(args->param);
	free(args);
	return NULL;
}

int main(void)
{
	pthread_rwlock_init(&rwlock, NULL);
	FILE *out = fopen("output.txt", "w");

	DIR *dirp = opendir("./");
	struct dirent *entry;
	char *filename = NULL;
	while ((entry = readdir(dirp)) != NULL)
	{
		if (strcmp(entry->d_name, "commands.txt") == 0)
		{
			filename = "commands.txt";
			break;
		}
	}
	closedir(dirp);
	if (!filename)
		filename = "dev/commands.txt";

	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "Could not open commands file.\n");
		return 1;
	}

	int num_commands = get_num_threads(fd);

	// Count inserts
	off_t pos = lseek(fd, 0, SEEK_CUR);
	for (int i = 0; i < num_commands; i++)
	{
		char *cmd = parse_until(fd, ',');
		if (strcmp(cmd, "insert") == 0)
			total_inserts++;
		free(cmd);
		parse_until(fd, ',');
		parse_until(fd, '\n');
	}
	lseek(fd, pos, SEEK_SET);

	pthread_t threads[num_commands];
	int thread_index = 0;

	for (int i = 0; i < num_commands; i++)
	{
		ThreadArgs *args = malloc(sizeof(ThreadArgs));
		args->command = parse_until(fd, ',');
		args->name = parse_until(fd, ',');
		args->param = parse_until(fd, '\n');
		args->out = out;

		pthread_create(&threads[thread_index++], NULL, handle_command, args);
	}

	for (int i = 0; i < thread_index; i++)
	{
		pthread_join(threads[i], NULL);
	}

	fprintf(out, "\nNumber of lock acquisitions: %d\n", lock_acquire_count);
	fprintf(out, "Number of lock releases: %d\n", lock_release_count);
	print_all(out);

	fclose(out);
	close(fd);
	pthread_rwlock_destroy(&rwlock);
	return 0;
}
