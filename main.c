#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 512
#define DELIMITERS " \t\n|><&;"

char *tokenList[50];

int getNumberOfTokens(char *input);
void printTokens(int noOfTokens);
void clearInputStream(void);
void executeExternalCommand(void);

int main (void) {
	// The operation of the shell should be as follows:
	
	// Find the user home directory from the environment
	char* origin = getenv("PATH");
	char* home = getenv("HOME");
	char cwd[512];
	// Set current working directory to user home directory
	getcwd(cwd, sizeof(cwd));
	printf("Current Working Dir: %s\n", cwd);
	if(chdir(home) != 0){
		printf("Set Directory Error");
	}	

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
			getcwd(cwd, sizeof(cwd));
			printf("Current Working Dir: %s\n", cwd);
			setenv("PATH", origin, 1);
			getcwd(cwd, sizeof(cwd));
			printf("Current Working Dir: %s\n", cwd);
			break;
		}
		if (strcmp(input, "\n") == 0){ // return to start of loop if there is no input
			continue;
		}

		//clear input stream if input is more than 512 characters
		if (strchr(input, '\n') == NULL){//check exist newline
			printf("ERROR: input contains to many characters. Input Max Length: 512 Characters\n");
			clearInputStream();
			continue;
		}

		//char *inputCopy = strdup(input); // copy input to new variable // Will be used when passing input to methods later on

		int noOfTokens = getNumberOfTokens(input); // store number of tokens from input

		if (noOfTokens == 0){ // return to start of loop if there is no tokens i.e Input only contains delimiters
			continue;
		}

		// If user entered exit with 0 zero arguments then break else error message
		if((strcmp(tokenList[0], "exit") == 0) && noOfTokens == 1){
			exit = true;
			getcwd(cwd, sizeof(cwd));
			printf("Current Working Dir: %s\n", cwd);
			chdir(origin);
			getcwd(cwd, sizeof(cwd));
			printf("Current Working Dir: %s\n", cwd);
			break;
		} else if ((strcmp(tokenList[0], "exit") == 0) && noOfTokens > 1){
				printf("ERROR: 'exit' does not take any arguments\n");
				continue;
		}
		
		//printTokens(noOfTokens); // Print each token for testing

		// While the command is a history invocation or alias then replace it with the
		// appropriate command from history or the aliased command respectively
		// If command is built-in invoke appropriate function
		// Else execute command as an external process
		executeExternalCommand();

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
	printf("Number of Tokens: %d\n", noOfTokens);
	for(int i = 0; i < noOfTokens; i++){
			printf("tokenList[%d]: \"%s\"\n" , i, tokenList[i]);
	}
}

void clearInputStream(void){
	while ((getchar()) != '\n'); //loop until new line has been found
}

void executeExternalCommand(void){
	pid_t pid;

	pid = fork();

	if(pid < 0){
		fprintf(stderr, "Fork Failed");
		return;
	} else if(pid == 0){
		execvp(tokenList[0], tokenList);
		perror(tokenList[0]);
		exit(EXIT_FAILURE);
	} else{
		wait(NULL);
	}
}