#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 512
#define DELIMITERS " \t\n|><&;"
#define MAX_PATH_LENGTH 512

void tokeinze(char *input, char *tokens[]);
int countTokens(char *tokens[]);
void printTokens(int noOfTokens, char *tokens[]);
void clearInputStream(void);
void executeExternalCommand(char *tokens[]);
void getDirectory(char dir[]);
void restorePath(char *path);

int main (void) {
	// The operation of the shell should be as follows:

	// Save the current path
	char *orginalPath = getenv("PATH");
	printf("Original Path: %s\n", getenv("PATH"));
	// Find the user home directory from the environment
	char *home = getenv("HOME");
	char directory[MAX_PATH_LENGTH];
	// Set current working directory to user home directory
	getDirectory(directory);
	printf("Starting Directory: %s\n", directory);
	chdir(home);
	getDirectory(directory);
	printf("Home Directory Set: %s\n", directory);
	// Load history
	// Load aliases

	char *tokenArray[51];
	char input[MAX_INPUT_LENGTH];
	
	// Do while shell has not terminated
	while(1){

		// Display prompt
		printf("> ");

		// Read and parse user input
		fgets(input, MAX_INPUT_LENGTH, stdin); // Read Input
		
		// If statement to exit when <ctrl>-D pressed
		if (feof(stdin) != 0){
			printf("\n");
			restorePath(orginalPath);
			break;
		}

		//clear input stream if input is more than 512 characters
		if (strchr(input, '\n') == NULL){//check exist newline
			printf("ERROR: input contains to many characters. Input Max Length: 512 Characters\n");
			clearInputStream();
			continue;
		}

		//char *inputCopy = strdup(input); // copy input to new variable // Will be used when passing input to methods later on

		tokeinze(input, tokenArray); // tokenize input
		int noOfTokens = countTokens(tokenArray); // store number of tokens from input

		if (tokenArray[0] == NULL){ // return to start of loop if there is no tokens i.e Input only contains delimiters
			continue;
		}

		// If user entered exit with 0 zero arguments then break else error message
		if((strcmp(tokenArray[0], "exit") == 0) && tokenArray[1] == NULL){
			restorePath(orginalPath);
			break;
		} else if ((strcmp(tokenArray[0], "exit") == 0) && tokenArray[1] != NULL){
				printf("ERROR: 'exit' does not take any arguments\n");
				continue;
		}
		// Go to start of loop if there is more than 50 tokens
		if(noOfTokens > 50){
			printf("ERROR: too many arguments. Max 50 tokens\n");
			continue;
		}
		
		//printTokens(noOfTokens, tokenArray); // Print each token for testing

		// While the command is a history invocation or alias then replace it with the
		// appropriate command from history or the aliased command respectively
		// If command is built-in invoke appropriate function
		// Else execute command as an external process
		executeExternalCommand(tokenArray);

	} // End while

	// Save history
	// Save aliases
	// Restore original path
	// Exit
	return 0;
}

void tokeinze(char *input, char *tokens[]){
	char *token;
	token = strtok(input, DELIMITERS);
	int index = 0;
	while (token != NULL){
		tokens[index] = token;
		token = strtok(NULL, DELIMITERS);
		index++;
	}
	tokens[index] = NULL;
}

int countTokens(char *tokens[]){
	int i = 0;
	while(tokens[i] != NULL){
		i++;
	}
	return i;
}

void printTokens(int noOfTokens, char *tokens[]){
	printf("Number of Tokens: %d\n", noOfTokens);
	for(int i = 0; i < noOfTokens; i++){
			printf("tokenArray[%d]: \"%s\"\n" , i, tokens[i]);
	}
}

void clearInputStream(void){
	while ((getchar()) != '\n'); //loop until new line has been found
}

void executeExternalCommand(char *tokens[]){
	pid_t pid;

	pid = fork();

	if(pid < 0){
		fprintf(stderr, "Fork Failed");
		return;
	} else if(pid == 0){
		execvp(tokens[0], tokens);
		perror(tokens[0]);
		exit(EXIT_FAILURE);
	} else{
		wait(NULL);
	}
}

void getDirectory(char dir[]){
	getcwd(dir, MAX_PATH_LENGTH);//add error
}

void restorePath(char *path){
	printf("Path before restore: %s\n", getenv("PATH"));
	setenv("PATH", path, 1); //add error
	printf("Path after restore: %s\n", getenv("PATH"));
}