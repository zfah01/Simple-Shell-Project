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
void runCommand(char *tokens[], char *history[], int *counter);
void executeExternalCommand(char *tokens[]);
void getDirectory(char dir[]);
void restorePath(char *path);
void getpathCommand(char *tokens[]);
void setpathCommand(char *tokens[]);
void cdCommand(char *tokens[]);
void addToHistory(char *input, char *history[], int *counter);
void printHistory(char *history[], int *counter);

int main (void) {
	// The operation of the shell should be as follows:

	// Save the current path
	char *orginalPath = getenv("PATH");
	// Find the user home directory from the environment
	char *home = getenv("HOME");
	// Set current working directory to user home directory
	chdir(home);
	// Load history
	char *history[20];
	for(int i = 0; i <= 20; i++){
		history[i] = malloc(sizeof(char) * MAX_INPUT_LENGTH);
		history[i] = "\0";
	}
	int counter = 0;
	// Load aliases

	char *tokenArray[51];
	char input[MAX_INPUT_LENGTH];
	char directory[MAX_PATH_LENGTH];
	
	// Do while shell has not terminated
	while(1){

		// Display prompt
		getDirectory(directory);
		printf("%s > ", directory);

		// Read and parse user input
		fgets(input, MAX_INPUT_LENGTH, stdin); // Read Input
		
		// If statement to exit when <ctrl>-D pressed
		if (feof(stdin) != 0){
			printf("\n");
			break;
		}

		//clear input stream if input is more than 512 characters
		if (strchr(input, '\n') == NULL){//check exist newline
			printf("ERROR: input contains to many characters. Input Max Length: 512 Characters\n");
			clearInputStream();
			continue;
		}

		char *inputCopy = strdup(input); // copy input to new variable // Will be used when passing input to methods later on
		printf("inputcopy: %s", inputCopy);
		tokeinze(input, tokenArray); // tokenize input
		int noOfTokens = countTokens(tokenArray); // store number of tokens from input

		if (tokenArray[0] == NULL){ // return to start of loop if there is no tokens i.e Input only contains delimiters
			continue;
		}

		// If user entered exit with 0 zero arguments then break else error message
		if((strcmp(tokenArray[0], "exit") == 0) && tokenArray[1] == NULL){
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
		addToHistory(inputCopy, history, &counter);
		// If command is built-in invoke appropriate function
		// Else execute command as an external process
		runCommand(tokenArray, history, &counter);

	} // End while

	// Save history
	// Save aliases
	// Restore original path
	restorePath(orginalPath);
	// Exit
	/*for(int i = 0; i <= 20; i++){
		free(history[i]);
	}*/
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

void runCommand(char *tokens[], char *history[], int *counter){
	if(strcmp(tokens[0], "getpath") == 0){
		getpathCommand(tokens);
	} else if (strcmp(tokens[0], "setpath") == 0){
		setpathCommand(tokens);
	} else if (strcmp(tokens[0], "cd") == 0){
		cdCommand(tokens);
	} else if (strcmp(tokens[0], "history") == 0){
		printHistory(history, counter);
	} else {
		executeExternalCommand(tokens);
	}
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
	if(getcwd(dir, MAX_PATH_LENGTH) == NULL){
		fprintf(stderr, "Error: Cannot get directory\n");
	}
}

void restorePath(char *path){
	setenv("PATH", path, 1);
}

void getpathCommand(char *tokens[]){
	if(tokens[1] == NULL){
		printf("Path: %s\n", getenv("PATH"));
	} else {
		fprintf(stderr, "getpath: Takes zero arguments");
	}
}

void setpathCommand(char *tokens[]){
	if(tokens[1] == NULL){
		printf("Error: setpath requires one argument\n");
	} else if (tokens[2] != NULL ){
		printf("Error: setpath only takes one argument\n");
	} else {
		setenv("PATH", tokens[1], 1);
		printf("Path set to: %s\n", getenv("PATH"));
	}
}

void cdCommand(char *tokens[]){
	char directory[MAX_PATH_LENGTH];
	if(tokens[1] == NULL){
		getDirectory(directory);
		printf("Directory before command: %s\n", directory);
		chdir(getenv("HOME"));
		getDirectory(directory);
		printf("Directory after command: %s\n", directory);
	} else if (tokens[2] == NULL){
	 	getDirectory(directory);
		if(chdir(tokens[1]) != 0){
			perror(tokens[1]);
		} else {
			printf("Directory before command: %s\n", directory);
			getDirectory(directory);
			printf("Directory after command: %s\n", directory);
		}
	} else {
		fprintf(stderr, "ERROR TOO MANY ARGUMENTS: the command 'cd' only takes one argument\n");
	}
}

void addToHistory(char *input, char *history[], int *counter){
	history[*counter] = strdup(input);
	*counter = ((*counter)+1) % 20;
}

void printHistory(char *history[], int *counter){
	int histNum = 1;
	int i = *counter;
	/*while(1){
		if(strcmp(history[i], "\0") != 0){
			printf("%d %s", histNum, history[i]);
		}
		i = (i+1) % 20;
		histNum++;
		if(i == *counter){
			break;
		}
	}*/
	for(int x = 1; x <= 20; x++){
		if(strcmp(history[x], "\0") != 0){
		printf("%d %s", x, history[x]);
		} else {
			break;
		}
	}
}