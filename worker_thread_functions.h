/*
	Collection of different operations that can be performed with a command
	
	The only non-static function is the one that should be called from outside this file, it picks which function to execute based on the char provided with the command
*/

#ifndef WTF
#define WTF

/* gets a command and a char indicating command type, what it will do with the command depends on the char
The return value depends on the type.
Note that none of the options change anything about the command itself
The avilable options are:
p	print the command to the screen and return NULL
s	print the command to the screen, sleep for 1 second, and then return NULL
TODO: add more
If none of the above are given, an error message is printed and NULL is returned */
void* commandHandler(char* command, char type);

#endif
