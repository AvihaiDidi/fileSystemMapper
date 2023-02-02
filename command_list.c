/*

Function documentation (comments) is in the header file
This file just has the implementations

(The static functions do have documentation here)

*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "command_list.h"
#include "worker_thread_functions.h"

char UTF8_BEEP = 7;
int BUFFER_SIZE = 10000; // beeg, so buffer overflow is impossible :') (jk, this is bad)
int BITS_IN_BYTE = 8; // very important const

// BEEP FUNCTION YEAAAAAAHHHHHHHHHHHHHHHHHHHHHH
void BEEP() {
	printf("%c", UTF8_BEEP);
}

// a struct that exists specifically to pass arguements to the thread function
typedef struct command_args {
	coms* c;
	char* command;
	int worker_id;
	char command_type;
} comargs;

// allocates more memory for future commands to be added, this should be done when len==clen
static void doubleClen(coms* c) {
	c->clen *= 2;
	char** temp = malloc(sizeof(char*) * c->clen);
	for (int i=0;i<c->len;i++)
		temp[i] = c->commands[i];
	free(c->commands);
	c->commands = temp;
}

// returns the length of s, \0 and \n are BOTH considered valid endpoints for the string
// if neither is present it'll probably segfault
static int strLen(char* s) {
	if (s[0] == '\0' || s[0] == '\n')
		return 0;
	char c;
	int i = 0;
	do {
		c = s[i++];
	} while (c != '\0' && s[0] != '\n');
	return i;
}

// copies src and returns the starting adress, src isn't modified
static char* copyStr(char* src) {
	char c;
	int i = 0;
	char* tar = malloc(sizeof(char)*(strLen(src) + 1));
	do {
		c = src[i];
		if (c == '\n') {
			tar[i] = '\0';
			break;
		}
		tar[i++] = c;
	} while (c != '\0' && c != '\n');
	return tar;
}

// toggles the activity status of a thread, this should be called whenever a thread starts executing a command (by the function starting the thread) and whenever the thread finishes it's command (by the thread itself)
static void toggleActive(coms* c, int id) {
	pthread_mutex_lock(&c->lock);
	if (c->thread_count <= id || id < 0) {
		printf("%d\t is a bad thread id\n", id);
		pthread_mutex_unlock(&c->lock);
		return;
	}
	c->activet = c->activet ^ (1 << id);
	pthread_mutex_unlock(&c->lock);
}

// returns the index of an active thread, if all threads are inactive returns -1
static int getActiveThreadId(coms* c) {
	int mask = 1;
	pthread_mutex_lock(&c->lock);
	for (int i=0;i<c->thread_count;i++) {
		if (mask & c->activet)
			return i;
		mask = mask << 1;
	}
	pthread_mutex_unlock(&c->lock);
	return -1;
}

// returns the index of an inactive thread, if all threads are active returns -1
static int getInactiveThreadId(coms* c) {
	int mask = 1;
	pthread_mutex_lock(&c->lock);
	for (int i=0;i<c->thread_count;i++) {
		if (mask & ~c->activet) {
			pthread_mutex_unlock(&c->lock);
			return i;
		}
		mask = mask << 1;
	}
	pthread_mutex_unlock(&c->lock);
	return -1;
}

/*	the function that the worker threads will execute
	uses the command_args (comargs) struct to pass arguements */
static void* workerThreadFunc(void* args) {
	// do the cast in advance so you won't have to repeat it
	comargs* cargs = (comargs*)args;
	// Do command
	void* ret = commandHandler(cargs->command, cargs->command_type);
	// set self to inactive and quit
	toggleActive(cargs->c, cargs->worker_id);
	// if all threads were taken, this next line will mark that a thread has finished executing and can be used for something else
	// otherwise, it just does nothing
	pthread_mutex_unlock(&(cargs->c->all_taken));
	free(cargs);
	return ret;
}

coms* initComs(int thread_count) {
	if (sizeof(bitarray) * BITS_IN_BYTE < thread_count) {
		printf("activet is represented using an unsigned int and can only keep track of %lu threads, %d is too high for it.\n", sizeof(unsigned int) * BITS_IN_BYTE, thread_count);
		return NULL;
	}
	coms* c = malloc(sizeof(coms));
	c->len = 0;
	c->clen = 1;
	if (pthread_mutex_init(&c->lock, NULL) != 0) {
		printf("mutex init failed, stoopid\n");
		free(c);
		return NULL;
	}
	c->commands = malloc(sizeof(char*) * c->clen);
	c->thread_count = thread_count;
	c->threads = malloc(sizeof(pthread_t)*c->thread_count);
	c->activet = 0;
	return c;
}

void waitForFinish(coms* c) {
	for (int i=0;i<c->thread_count;i++)
		pthread_join(c->threads[i], NULL);
}

void killComs(coms* c) {
	free(c->threads);
	for (int i=0;i<c->len;i++)
		free(c->commands[i]);
	free(c->commands);
	pthread_mutex_destroy(&c->lock);
	pthread_mutex_destroy(&c->all_taken);
	free(c);
}

void printComs(coms* c) {
	pthread_mutex_lock(&c->lock);
	for (int i=0;i<c->len;i++)
		printf("%d\t%s\n", i+1, c->commands[i]);
	pthread_mutex_unlock(&c->lock);
}

char* popCommand(coms* c) {
	pthread_mutex_lock(&c->lock);
	if (c->len == 0) {
		pthread_mutex_unlock(&c->lock);
		return NULL;
	}
	char* command = c->commands[c->len - 1];
	c->len--;
	pthread_mutex_unlock(&c->lock);
	return command;
}

void addCommand(coms* c, char* command) {
	pthread_mutex_lock(&c->lock);
	c->commands[c->len] = copyStr(command);
	c->len++;
	if (c->len == c->clen)
		doubleClen(c);
	pthread_mutex_unlock(&c->lock);
}

void addCommands(coms* c, char** commands, int len) {
	pthread_mutex_lock(&c->lock);
	// check if the length should be doubled in advance, rather than each time
	if (c->clen <= c->len + len)
		doubleClen(c);
	for (int i=0; i<len; i++)
		c->commands[c->len + i] = copyStr(commands[i]);
	c->len += len;
	pthread_mutex_unlock(&c->lock);
}

void addCommandsFromFile(coms* c, char* path) {
	FILE* f;
	char* buffer = malloc(BUFFER_SIZE * sizeof(char));
	f = fopen(path, "r");
	if (f == NULL) {
		printf("Failed to open the file '%s'.\n", path);
		free(buffer);
	}
	pthread_mutex_lock(&c->lock);
	while (fgets(buffer, BUFFER_SIZE, f) != NULL) {
		c->commands[c->len] = copyStr(buffer);
		c->len++;
		if (c->len == c->clen)
			doubleClen(c);
	}
	pthread_mutex_unlock(&c->lock);
	free(buffer);
	fclose(f);
}

void processList(coms* c, char type) {
	while (0 < c->len) {
		comargs* args = malloc(sizeof(comargs));
		args->c = c;
		args->command = popCommand(c);
		args->command_type = type;
		int temp_id = getInactiveThreadId(c);
		if (temp_id == -1) {
			pthread_mutex_lock(&c->all_taken); // wait for a thread to finish
			temp_id  = getInactiveThreadId(c);
		}
		toggleActive(c, temp_id);
		if (getInactiveThreadId(c) == -1)
			pthread_mutex_lock(&c->all_taken);
		args->worker_id = temp_id;
		pthread_create(&c->threads[temp_id], NULL, workerThreadFunc, (void*)args);
	}
}
