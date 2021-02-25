#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_INPUT_LENGTH 512
#define MAX_TOKENS 51
#define DELIMITERS " \t\n|><&;"
#define MAX_PATH_LENGTH 512
#define MAX_HISTORY_SIZE 20
#define HISTORY_FILE_NAME ".hist_list"
#define MAX_ALIAS_SIZE 10
#define MAX_ALIAS_ELEMENTS 2

void tokeinze(char *input, char *tokens[], char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]);
int countTokens(char *tokens[]);
void printTokens(char *tokens[]);
void clearInputStream(void);
void runCommand(char *tokens[], char *history[], int counter, int historyHead, char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]);
void executeExternalCommand(char *tokens[]);
void getDirectory(char dir[]);
void restorePath(char *path);
void getpathCommand(char *tokens[]);
void setpathCommand(char *tokens[]);
void cdCommand(char *tokens[]);
void addToHistory(char *input, char *history[], int *counter, int *historyHead);
void printHistory(char *history[], int counter, int historyHead);
int getHistoryCallIndex(char *history[], int counter, char *tokens[], int historyHead);
int isHistoryCmdValid(char *str);
int getHistorySize(char *history[]);
void loadHistory(char *history[], int *counter, int *historyHead);
void saveHistory(char *history[], int counter, int historyHead);
void addAlias(char *tokens[], char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]);
void removeAlias(char *tokens[], char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]);
void printAliases(char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]);
int isAlias(char *token, char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]);

int main (void) {
	// The operation of the shell should be as follows:

	// Save the current path
	char *orginalPath = getenv("PATH");
	// Find the user home directory from the environment
	char *home = getenv("HOME");
	// Set current working directory to user home directory
	chdir(home);
	// Load history
	char *history[MAX_HISTORY_SIZE];
	int counter = 0;
	int historyHead = 0;
	loadHistory(history, &counter, &historyHead);
	// Load aliases
	char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS];

	char *tokenArray[MAX_TOKENS];
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
		tokeinze(input, tokenArray, aliasArray); // tokenize input
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
		
		//printTokens(tokenArray); // Print each token for testing

		// While the command is a history invocation or alias then replace it with the
		// appropriate command from history or the aliased command respectively
		if(strcspn(tokenArray[0], "!") == 0){ // if history invocation 
			int historyIndex = getHistoryCallIndex(history, counter, tokenArray, historyHead); // get index for history array. returns -1 if failed
			if (historyIndex != -1){
				char *historyInput = strdup(history[historyIndex]); // store cmd from history
				tokeinze(historyInput, tokenArray, aliasArray); // tokenize cmd from history
			} else {
				continue;
			}
		} else {
			addToHistory(inputCopy, history, &counter, &historyHead);
		}
		// If command is built-in invoke appropriate function
		// Else execute command as an external process
		runCommand(tokenArray, history, counter, historyHead, aliasArray);

	} // End while

	// Save history
	chdir(home);
	saveHistory(history, counter, historyHead);
	// Save aliases
	// Restore original path
	restorePath(orginalPath);
	// Exit
	return 0;
}

void tokeinze(char *input, char *tokens[], char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]){
	char *token;
	token = strtok(input, DELIMITERS);
	int index = 0;
	int aliasIndex = -1;
	while (token != NULL){
		aliasIndex = isAlias(token, aliasArray);
		if(aliasIndex >= 0){
			printf("replacing: \"%s\" with: \"%s\"\n", token, aliasArray[aliasIndex][1]);
			token = strdup(aliasArray[aliasIndex][1]);
		}
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

void printTokens(char *tokens[]){
	int noOfTokens = countTokens(tokens);
	printf("Number of Tokens: %d\n", noOfTokens);
	for(int i = 0; i < noOfTokens; i++){
			printf("tokenArray[%d]: \"%s\"\n" , i, tokens[i]);
	}
}

void clearInputStream(void){
	while ((getchar()) != '\n'); //loop until new line has been found
}

void runCommand(char *tokens[], char *history[], int counter, int historyHead, char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]){
	if(strcmp(tokens[0], "getpath") == 0){
		getpathCommand(tokens);
	} else if (strcmp(tokens[0], "setpath") == 0){
		setpathCommand(tokens);
	} else if (strcmp(tokens[0], "cd") == 0){
		cdCommand(tokens);
	} else if (strcmp(tokens[0], "history") == 0){
		if(tokens[1] == NULL){
			printHistory(history, counter, historyHead);
		} else {
			fprintf(stderr, "%s: does not take arguments\n", tokens[0]);
		}
	} else if (strcmp(tokens[0], "alias") == 0){
		if (tokens[1] == NULL){
			printAliases(aliasArray);
		} else {
			addAlias(tokens, aliasArray);
		} 
	} else if (strcmp(tokens[0], "unalias") == 0){
		removeAlias(tokens, aliasArray);
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
	if(tokens[1] == NULL){
		chdir(getenv("HOME"));
	} else if (tokens[2] == NULL){
		if(chdir(tokens[1]) != 0){
			perror(tokens[1]);
		}
	} else {
		fprintf(stderr, "ERROR TOO MANY ARGUMENTS: the command 'cd' only takes one argument\n");
	}
}

void addToHistory(char *input, char *history[], int *counter, int *historyHead){
	if (history[19] != NULL && *historyHead == 0){ // if the last element is not null and the head 0 set head to 1
		*historyHead = 1;
	} else if (*historyHead >= 1){ // continue to add to history head each time once 20 elements have been stored
		*historyHead = (*historyHead+1) % MAX_HISTORY_SIZE;
	}
	if(input[strlen(input)-1] == '\n' ){ // if last char in input is \n remove it
    input[strlen(input)-1] = '\0';
	}
	history[*counter] = strdup(input);
	*counter = (*counter+1) % MAX_HISTORY_SIZE;
}

void printHistory(char *history[], int counter, int historyHead){

	int histNum = 1;
	int index = historyHead;
	do {
		printf("%d %s\n", histNum, history[index]);
		histNum++;
		index = (index+1) % MAX_HISTORY_SIZE;
	} while (index != counter);

}

int getHistoryCallIndex(char *history[], int counter, char *tokens[], int historyHead){
	int index;
	int historySize = getHistorySize(history);
	if(history[0] == NULL){
		fprintf(stderr, "ERROR: history is empty\n");
		return -1;
	}else if(tokens[1] != NULL){
		fprintf(stderr, "%s: does not take any arguments\n", tokens[0]);
		return -1;
	} else if (strcmp(tokens[0], "!!") == 0){
		index = (historyHead+(historySize-1)) % MAX_HISTORY_SIZE;
	} else if(isHistoryCmdValid(tokens[0]) == 0){
		int x;
		sscanf(tokens[0], "!%d", &x);
		if(x < 0){
			x = abs(x); // change negative to positve easier to read. a - (x) instead of a + (-x)
			if(x >= historySize){
				fprintf(stderr ,"ERROR: the subtraction from history must be less than the history size: %d\n", historySize);
				return -1;
			}
			counter = (historySize-(1%historySize) + counter)%historySize; //set counter to the index of the last filled index in history instead of the next one to fill
			index = (historySize-(x%historySize) + counter)%historySize; // calculate index for circular array with subtraction from last index
		} else {
			if(x > 0 && x <= MAX_HISTORY_SIZE) {
			index = (historyHead+(x-1)) % MAX_HISTORY_SIZE;
			} else {
				index = x - 1;
			}
		}
	} else{
		fprintf(stderr, "%s: not understood\n", tokens[0]);
		return-1;
	}

	if(index < 0 || index >= MAX_HISTORY_SIZE){
		fprintf(stderr, "ERROR: history number must be between 1 and %d\n", MAX_HISTORY_SIZE);
		return-1;
	} else if(history[index] == NULL){
		fprintf(stderr, "ERROR: %s event not found\n", tokens[0]);
		return-1;
	}
	return index;
}

int isHistoryCmdValid(char *str){
	if(strlen(str) < 2){
		return 1;
	}
	for(int i = 1; i < strlen(str); str++){
		if(!((str[i] == '-' && i == 1) || (isdigit(str[i]) != 0))){
			return 1;
		}
	}
	return 0;
}

int getHistorySize(char *history[]){
	int size = 0;
	while(size < MAX_HISTORY_SIZE && history[size] != NULL){
		size++;
	}
	return size;
}

void loadHistory(char *history[], int *counter, int *historyHead){

	FILE *fptr;
	fptr = fopen(HISTORY_FILE_NAME, "r");

	if(fptr == NULL){
		return;
	}

	int counterCopy = *counter;
	int historyHeadCopy = *historyHead;

	if(fptr != NULL){
		char line[MAX_INPUT_LENGTH];
		while(fgets(line, MAX_INPUT_LENGTH, fptr) != NULL){
			if(line[strlen(line)-1] == '\n' ){ // if last char in input is \n remove it
    			line[strlen(line)-1] = '\0';
			}
			addToHistory(line, history, &counterCopy, &historyHeadCopy);
		}
	}
	
	*counter = counterCopy;
	*historyHead = historyHeadCopy;

	fclose(fptr);

}

void saveHistory(char *history[], int counter, int historyHead){

	if(history[0] == NULL){
		return;
	}

	FILE *fptr = fopen(HISTORY_FILE_NAME, "w");
	if(fptr != NULL){
		int histNum = 1;
		int index = historyHead;
		do {
		fprintf(fptr, "%s\n", history[index]);
		histNum++;
		index = (index+1) % MAX_HISTORY_SIZE;
		} while (index != counter);
	}
	fclose(fptr);

}

void addAlias(char *tokens[], char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]){

	if (tokens[2] == NULL){
		fprintf(stderr, "ERROR: alias \"%s\" requires a command\n", tokens[1]);
	} else if (tokens[3] == NULL){
		// this loop checks if the alias exists then updates
		for(int i = 0; i < MAX_ALIAS_SIZE; i++){
			if((aliasArray[i][0] != NULL) && (strcmp(aliasArray[i][0], tokens[1]) == 0)){
				char *temp = strdup(aliasArray[i][1]);
				aliasArray[i][0] = strdup(tokens[1]);
				aliasArray[i][1] = strdup(tokens[2]);
				printf("alias updated: \"%s\", command: \"%s\", previous command: \"%s\"\n", aliasArray[i][0], aliasArray[i][1], temp);
				return;
			}
		}
		// this loop finds an empty space in the aliasArray and adds the alias
		for(int i = 0; i < MAX_ALIAS_SIZE; i++){
			if(aliasArray[i][0] == NULL){
				aliasArray[i][0] = strdup(tokens[1]);
				aliasArray[i][1] = strdup(tokens[2]);
				printf("alias: \"%s\" command: \"%s\"\n", aliasArray[i][0], aliasArray[i][1]);
				return;
			}
		}
		fprintf(stderr, "ERROR: no more aliases can be set\n");
	} else {
		fprintf(stderr, "ERROR: alias can take a max of three arguments\n");
		fprintf(stderr, "Suggestions: \"alias\" or \"alias <name> <command>\"\n");
	}

}

void removeAlias(char *tokens[], char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]){
	if(tokens[1] == NULL){
		fprintf(stderr, "unalias: requires one argument\n");
	} else if (tokens[2] == NULL){
		for(int i = 0; i < MAX_ALIAS_SIZE; i++){
			if((aliasArray[i][0] != NULL) && (strcmp(aliasArray[i][1], tokens[1]) == 0)){
				aliasArray[i][0] = NULL;
				aliasArray[i][1] = NULL;
				return;
			}
		}
		fprintf(stderr, "unalias: %s: not found\n", tokens[1]);
	} else {
		fprintf(stderr, "unalias: requires only one argument\n");
	}
}

void printAliases(char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]){

	int aliasPrinted = 0;

	for(int i = 0; i < MAX_ALIAS_SIZE; i++){
		if(aliasArray[i][0] != NULL && aliasArray[i][1] != NULL) {
			printf("alias %s='%s'\n", aliasArray[i][0], aliasArray[i][1]);
			aliasPrinted = 1;
		}
	}

	if(aliasPrinted == 0){
	fprintf(stderr, "ERROR: no aliases have been set up\n");
	}

}

int isAlias(char *token, char *aliasArray[MAX_ALIAS_SIZE][MAX_ALIAS_ELEMENTS]){

	for(int i = 0; i < MAX_ALIAS_SIZE; i++){
		if((aliasArray[i][0] != NULL) && (strcmp(aliasArray[i][0], token) == 0)) {
			return i;
		}
	}

	return -1;

}