#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

int main (void) {
	// The operation of the shell should be as follows:
	
	// Find the user home directory from the environment
	// Set current working directory to user home directory
	// Save the current path
	// Load history
	// Load aliases
	char input[512];
	bool exit = false;
	char* token;
	int tokenIndex = 0;
	char* tokenList[10];
	char delimit[] = "|><&; \t";
	// Do while shell has not terminated
	while(!exit){
		printf("> ");
		tokenIndex = 0;
		fgets(input, 512, stdin);
		// If statement to exit when <ctrl>-D pressed
		if (feof(stdin) != 0) { 
			exit = true;
			printf("\n");
			break;
		}
		token =  strtok(input, delimit);
		while(token != NULL){
			printf("%s\n", token);
			tokenList[tokenIndex++] = token;
			token = strtok(NULL, delimit);
		}

		if (strcmp(tokenList[0], "exit") == 0){
			exit = true;
		}

		free(token);
		
	}
		// Display prompt
		// Read and parse user input
		// While the command is a history invocation or alias then replace it with the
		// appropriate command from history or the aliased command respectively
		// If command is built-in invoke appropriate function
		// Else execute command as an external process
	// End while
	// Save history
	// Save aliases
	// Restore original path
	// Exit
	return 0;
}
