/*

Function documentation (comments) is in the header file
This file just has the implementations

(The static functions do have documentation here)

*/

#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include "worker_thread_functions.h"

// handles p type commands (see commandHandler documentation)
static void CHp(char* command) {
	printf("%s\n", command);
}

// handles s type commands (see commandHandler documentation)
static void CHs(char* command) {
	CHp(command);
	sleep(1);
}

void* commandHandler(char* command, char type) {
	switch (type) {
		case 'p':
			CHp(command);
			return NULL;
		break;
		case 's':
			CHs(command);
			return NULL;
		break;
		default:
			printf("Unsupported operation '%c', returning NULL\n", type);
			return NULL;
	}
}
