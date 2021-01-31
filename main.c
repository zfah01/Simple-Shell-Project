#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LENGTH 512
#define DELIMITERS " \t\n|><&;"

char *tokenList[50];

int getNumberOfTokens(char *input);
void printTokens(int noOfTokens);

int main (void) {
	// The operation of the shell should be as follows:
	
	// Find the user home directory from the environment
	// Set current working directory to user home directory
	// Save the current path
	// Load history
	// Load aliases

	char input[MAX_INPUT_LENGTH];
	bool exit = false;
	
	// Do while shell has not terminated
	while(!exit){

		// Display prompt
		printf("> ");

		// Read and parse user input
		fgets(input, MAX_INPUT_LENGTH, stdin); // Read Input
		
		// If statement to exit when <ctrl>-D pressed
		if (feof(stdin) != 0){
			exit = true;
			printf("\n");
			break;
		}
		if (strcmp(input, "\n") == 0){ // return to start of loop if there is no input
			continue;
		}

		//char *inputCopy = strdup(input); // copy input to new variable // Will be used when passing input to methods later on

		int noOfTokens = getNumberOfTokens(input); // store number of tokens from input
		printf("Number of Tokens: %d\n", noOfTokens); // Print Number of Tokens for Testing

		if (noOfTokens == 0){ // return to start of loop if there is no tokens i.e Input only contains delimiters
			continue;
		}

		// If user entered exit with 0 zero arguments then break else error message
		if((strcmp(tokenList[0], "exit") == 0) && noOfTokens == 1){
			exit = true;
			printf("\n");
			break;
		} else if ((strcmp(tokenList[0], "exit") == 0) && noOfTokens > 1){
				printf("ERROR: 'exit' does not take any arguments\n");
				continue;
		}
		
		
		printTokens(noOfTokens); // Print each token for testing

		// While the command is a history invocation or alias then replace it with the
		// appropriate command from history or the aliased command respectively
		// If command is built-in invoke appropriate function
		// Else execute command as an external process

	}
	// End while
	// Save history
	// Save aliases
	// Restore original path
	// Exit
	return 0;
}

int getNumberOfTokens(char *input){
	char *token;
	token = strtok(input, DELIMITERS);
	int x = 0;
	while (token != NULL){
		tokenList[x] = token;
		token = strtok(NULL, DELIMITERS);
		x++;
	}
	tokenList[x] = NULL;
	return x;
}

void printTokens(int noOfTokens){
	for(int i = 0; i < noOfTokens; i++){
			printf("tokenList[%d]: \"%s\"\n" , i, tokenList[i]);
	}
}