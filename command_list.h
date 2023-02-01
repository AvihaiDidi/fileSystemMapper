/*

This code is for defining a 'class' that handles a list of commands and performs them in multiple threads in parallal.
Note that in practice the list of commands functions like a stack.
Also note that, because threads, the order in which tasks are executed is NOT gaurnteed.

TODO: maybe make it function like a queue instead in the future

*/

#ifndef COMMANDLIST
#define COMMANDLIST

// this struct is the whole point of this file, it's a stack of commands, and threads that perform those commands
typedef struct command_list {
	int len;  // actual length of the commands list
	int clen; // length of the space allocated for command string pointers, must always be >= len
	char** commands;
	pthread_mutex_t lock; // lock for modifing the state of the command list
	pthread_mutex_t all_taken; // lock for when all threads are occupied, and you have to wait for some to become avilable
	int thread_count;
	pthread_t* threads;
	unsigned int activet;
} coms;

// EXTREMLY important function, it make funny beep
void BEEP();

// initiazlie a new command list struct, prints an error message and returns NULL if initialization failed for some reason
coms* initComs(int thread_count);

// waits for all active threads to finish executing
void waitForFinish(coms* c);

// frees all memory used by the command list
// note that it doesn't wait for any active threads to finish, and just deletes the pointers to those threads
void killComs(coms* c);

// removes an item from the top of the list and return it
// note that the responsibility of freeing the memory used by the command now falls on the code that called this function and not on the command list itself
char* popCommand(coms* c);

// Just prints all items currently in the list, in the order they were added (which is the opposite of the order that they will be executed, because the list functions like a stack)
void printComs(coms* c);

// adds a command to the top of the list
// DOES NOT free the memory used by the command
void addCommand(coms* c, char* command);

// adds a bunch of commands to the top of the list
// DOES NOT free the memory used by the commands
void addCommands(coms* c, char** commands, int len);

// gets an initialized coms pointer and a file path, adds all of the commands from path to c
void addCommandsFromFile(coms* c, char* path);

/*	this is the big important function
	while len > 0, pops commands and performs them
	type is the way the commands should be handeled (see the documentation in worker_thread_functions.h for details)
	note that this function DOESN'T WAIT FOR ALL THREADS TO EXIT, IT JUST ASSIGNS ALL OF THE WORK TO THE AVILABLE THREADS AND THEN EXITS
	this one should only run once in one thread, trying to use this function in several threads on the same command list will result in undefined behavior (i.e, it'll probably segfault) */
void processList(coms* c, char type);

#endif
