/**
 * CS210 Simple Shell Coursework
 * 
 * A simple shell program written in C that executes internal and external commands.
 * Making use of built-in Linux commands. The shell also keeps a history of user
 * commands and supports aliases, both of these features are persistent.
 * 
 * Compile:
 * gcc shell.c -pedantic -Wall -o shell
 * 
 * Run:
 * ./shell
 * 
 * CS210 2020/21 Semester 2 Cousrswork
 * 
 * Group 11:
 * Najeeb Asif
 * Stanley White
 * Ibrahim Safdar
 * Zain Faheem
 * Andreea Caragea
 */

// import external libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

// define constants
#define MAX_INPUT_LENGTH 512
#define MAX_TOKENS 51
#define DELIMITERS " \t\n|><&;"
#define MAX_PATH_LENGTH 512
#define MAX_HISTORY_SIZE 20
#define HISTORY_FILE_NAME ".hist_list"
#define MAX_ALIAS_KEY_LENGTH 30
#define MAX_ALIAS_TOKENS 49
#define MAX_ALIAS_SIZE 10
#define ALIAS_FILE_NAME ".aliases"
#define MAX_ALIAS_RECURSION_DEPTH 10
#define INITIAL_ALIAS_RECURSION_DEPTH 0

// define alias struct
typedef struct ALIAS {
    char aliasKey[MAX_ALIAS_KEY_LENGTH];
    char aliasCommands[MAX_ALIAS_TOKENS][MAX_INPUT_LENGTH];
    int noOfCommands;
} alias;

// function definitions
void tokeinze(char *input, char *tokens[], alias aliasArray[MAX_ALIAS_SIZE], int recurrsionDepth);
int countTokens(char *tokens[]);
void clearInputStream(void);
void runCommand(char *tokens[], char *history[], int historyCounter, int historyHead, alias aliasArray[MAX_ALIAS_SIZE]);
void executeExternalCommand(char *tokens[]);
void getDirectory(char dir[]);
void restorePath(char *path);
void getpathCommand(char *tokens[]);
void setpathCommand(char *tokens[]);
void cdCommand(char *tokens[]);
void addToHistory(char *input, char *history[], int *historyCounter, int *historyHead);
void printHistory(char *history[], int historyCounter, int historyHead);
int getHistoryCallIndex(char *history[], int historyCounter, char *tokens[], int historyHead);
int isHistoryCmdValid(char *str);
int getHistorySize(char *history[]);
void loadHistory(char *history[], int *historyCounter, int *historyHead);
void saveHistory(char *history[], int historyCounter, int historyHead);
void initialiseAliasKeys(alias aliasArray[MAX_ALIAS_SIZE]);
void addAlias(char *tokens[], alias aliasArray[MAX_ALIAS_SIZE]);
void removeAlias(char *tokens[], alias aliasArray[MAX_ALIAS_SIZE]);
void printAliases(alias aliasArray[MAX_ALIAS_SIZE]);
int isAlias(char *token, alias aliasArray[MAX_ALIAS_SIZE]);
void loadAliases(alias aliasArray[MAX_ALIAS_SIZE]);
void saveAliases(alias aliasArray[MAX_ALIAS_SIZE]);
int checkAliasExistInTokens(char *tokens[], alias aliasArray[MAX_ALIAS_SIZE]);

int main(void) {

    // Save the current path
    char *orginalPath = getenv("PATH");

    // Find the user home directory from the environment
    char *home = getenv("HOME");
    // Set current working directory to user home directory
    chdir(home);
    
    // Load history
    char *history[MAX_HISTORY_SIZE];
    int historyCounter = 0; // stores the next availble inext in the history array
    int historyHead = 0; // stores the index value of !1
    loadHistory(history, &historyCounter, &historyHead);

    // Load aliases
    alias aliasArray[MAX_ALIAS_SIZE];
    initialiseAliasKeys(aliasArray);
    loadAliases(aliasArray);

    char *tokenArray[MAX_TOKENS]; // array of string to store tokenized input
    char input[MAX_INPUT_LENGTH]; // string to store user input
    char directory[MAX_PATH_LENGTH]; // string to store current directory

    // Do while shell has not terminated
    while (1) {

        // Display prompt
        getDirectory(directory);
        printf("%s > ", directory);

        // Read Input
        fgets(input, MAX_INPUT_LENGTH, stdin); 

        // If statement to exit loop when <ctrl>-D pressed
        if (feof(stdin) != 0) {
            printf("\n");
            break;
        }

        //clear input stream if input is more than 512 characters
        if (strchr(input, '\n') == NULL) { //check if newline exists 
            fprintf(stderr, "ERROR: input contains too many characters. Input Max Length: 512 Characters\n");
            clearInputStream();
            continue;
        }

        char *inputCopy = strdup(input); // copy input string before to tokenization for history
        tokeinze(input, tokenArray, aliasArray, INITIAL_ALIAS_RECURSION_DEPTH);  // tokenize input also checks for aliases and replaces tokens
        int noOfTokens = countTokens(tokenArray); // store number of tokens from input

        if (tokenArray[0] == NULL) { // return to start of loop if there is no tokens i.e Input only contains delimiters
            continue;
        }

        // If user entered exit with 0 zero arguments then exit loop else error message
        if ((strcmp(tokenArray[0], "exit") == 0) && tokenArray[1] == NULL) {
            break;
        }
        else if ((strcmp(tokenArray[0], "exit") == 0) && tokenArray[1] != NULL) {
            fprintf(stderr, "ERROR: 'exit' does not take any arguments\n");
            continue;
        }

        // Go to start of loop if there is more than 50 tokens
        if (noOfTokens > 50) {
            fprintf(stderr, "ERROR: too many arguments. Max 50 tokens\n");
            continue;
        }

        // check if input was history invocation
        if (strcspn(tokenArray[0], "!") == 0) {
            int historyIndex = getHistoryCallIndex(history, historyCounter, tokenArray, historyHead); // get index for history array. returns -1 if failed
            if (historyIndex != -1) {
                char *historyInput = strdup(history[historyIndex]); // store cmd from history
                tokeinze(historyInput, tokenArray, aliasArray, INITIAL_ALIAS_RECURSION_DEPTH); // tokenize cmd from history
            }
            else {
                continue;
            }
        }
        else { // else add input to history
            addToHistory(inputCopy, history, &historyCounter, &historyHead);
        }

        // run the appropriate command
        runCommand(tokenArray, history, historyCounter, historyHead, aliasArray);

    } // End while

    // Operations for exiting the shell:
    // Save history
    chdir(home); // change directory to home
    saveHistory(history, historyCounter, historyHead);
    // Save aliases
    saveAliases(aliasArray);
    // Restore original path
    restorePath(orginalPath);
    // Exit
    return 0;
}

/**
 * This function tokenizes the input into an array of strings. With the array being NULL terminated.
 * While tokenizing if an alias is found it will be replaced with the alias tokens.
 * If there are still aliases to be replaced at the end then a recurrsive call is used.
 */
void tokeinze(char *input, char *tokens[], alias aliasArray[MAX_ALIAS_SIZE], int recurrsionDepth) {

    //exit if the max recurrsive depth is reached
    if (recurrsionDepth == MAX_ALIAS_RECURSION_DEPTH) {
        tokens[0] = NULL;
        fprintf(stderr, "ERROR: alias recurrsion depth reached\n");
        return;
    }

    char *token;
    token = strtok(input, DELIMITERS); // get the first token
    int index = 0;

    // if first token is alias or unalias then don't replace aliases
    if ((token != NULL) && ((strcmp(token, "alias") == 0) || (strcmp(token, "unalias") == 0))) {
        while (token != NULL) {
            tokens[index] = token;
            token = strtok(NULL, DELIMITERS);
            index++;
        }
        tokens[index] = NULL;
    }
    else {
        int aliasIndex = -1;
        while (token != NULL) {
            aliasIndex = isAlias(token, aliasArray); // returns >= 0 if alias exists
            if (aliasIndex >= 0) { // insert alias tokens if token is alias
                for (int i = 0; i < aliasArray[aliasIndex].noOfCommands; i++) {
                    tokens[index] = aliasArray[aliasIndex].aliasCommands[i];
                    index++;
                }
            }
            else { // else insert the token from the input into the token array
                tokens[index] = token;
                index++;
            }
            token = strtok(NULL, DELIMITERS); // continue to tokenize the string
        }
        tokens[index] = NULL; // NULL terminate the array

        // check if aliases still exist
        if (checkAliasExistInTokens(tokens, aliasArray) == 0){
            index = 0;
            // concatenate the tokens back into one string
            char concatenatedString[MAX_INPUT_LENGTH] = "\0";
            while (tokens[index] != NULL) {
                strcat(concatenatedString, tokens[index]);
                strcat(concatenatedString, " ");
                index++;
            }
            // recursive call with one added to the recurrsion depth
            tokeinze(concatenatedString, tokens, aliasArray, recurrsionDepth + 1);
        }
    }
}

/**
 * Function to return the number of tokens in a NULL terminated array of strings
 */
int countTokens(char *tokens[]) {
    int i = 0;
    while (tokens[i] != NULL) {
        i++;
    }
    return i;
}

/**
 * Fuction to clear stdin
 */
void clearInputStream(void) {
    while ((getchar()) != '\n') // get the next char from stdin
        ; //loop until new line has been found
}

/**
 * This function finds the appropriate command to execute
 */
void runCommand(char *tokens[], char *history[], int historyCounter, int historyHead, alias aliasArray[MAX_ALIAS_SIZE]) {

    // If command is built-in invoke appropriate function
    // Else execute command as an external process
    if (strcmp(tokens[0], "getpath") == 0) {
        getpathCommand(tokens);
    }
    else if (strcmp(tokens[0], "setpath") == 0) {
        setpathCommand(tokens);
    }
    else if (strcmp(tokens[0], "cd") == 0) {
        cdCommand(tokens);
    }
    else if (strcmp(tokens[0], "history") == 0) {
        if (tokens[1] == NULL) {
            printHistory(history, historyCounter, historyHead);
        }
        else {
            fprintf(stderr, "%s: does not take arguments\n", tokens[0]);
        }
    }
    else if (strcmp(tokens[0], "alias") == 0) {
        if (tokens[1] == NULL) {
            printAliases(aliasArray);
        }
        else {
            addAlias(tokens, aliasArray);
        }
    }
    else if (strcmp(tokens[0], "unalias") == 0) {
        removeAlias(tokens, aliasArray);
    }
    else {
        executeExternalCommand(tokens);
    }
}

/**
 * This function executes an external process using fork and exec
 */
void executeExternalCommand(char *tokens[]) {
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return;
    }
    else if (pid == 0) {
        execvp(tokens[0], tokens); // execute child process
        perror(tokens[0]); // print appropriate error 
        exit(EXIT_FAILURE);
    }
    else {
        wait(NULL);
    }
}

/**
 * This function gets the current directory and stores it in the string argument passed in
 */
void getDirectory(char dir[]) {
    if (getcwd(dir, MAX_PATH_LENGTH) == NULL) {
        fprintf(stderr, "Error: Cannot get directory\n");
    }
}

/**
 * This function restores the path variable
 */
void restorePath(char *path) {
    setenv("PATH", path, 1);
}

/**
 * This function prints the current path variable on the screen.
 * getpath command
 */
void getpathCommand(char *tokens[]) {
    if (tokens[1] == NULL) { // getpath input should only be one toekn long
        printf("Path: %s\n", getenv("PATH"));
    }
    else {
        fprintf(stderr, "getpath: Takes zero arguments");
    }
}

/**
 * This function sets the path variable with users input from the tokens array.
 * setpath command
 */
void setpathCommand(char *tokens[]) {
    // setpath command must be two tokens long include the command
    if (tokens[1] == NULL) {
        fprintf(stderr, "Error: setpath requires only one argument\n");
    }
    else if (tokens[2] != NULL) {
        fprintf(stderr, "Error: setpath only takes one argument\n");
    }
    else {
        setenv("PATH", tokens[1], 1); // function to set the path variable
        printf("Path set to: %s\n", getenv("PATH")); // print the new path
    }
}

/**
 * This function executes the cd command
 */
void cdCommand(char *tokens[]) {
    if (tokens[1] == NULL) { // if there are zero arguments set the directory to home
        chdir(getenv("HOME"));
    }
    else if (tokens[2] == NULL) {
        if (chdir(tokens[1]) != 0) { // change directory if there is only one argument to cd
            perror(tokens[1]); // print error if chdir failed
        }
    }
    else { // print error if more than one argument
        fprintf(stderr, "ERROR TOO MANY ARGUMENTS: the command 'cd' only takes one argument\n");
    }
}

/**
 * Function adds user input to history
 */
void addToHistory(char *input, char *history[], int *historyCounter, int *historyHead) {
    if (history[19] != NULL && *historyHead == 0) { // if the last element is not null and the head 0 set head to 1
        *historyHead = 1;                           // i.e there is now 20 elements in the history array
    }
    else if (*historyHead >= 1) { // continue to add to history head each time once 20 elements have been stored
        *historyHead = (*historyHead + 1) % MAX_HISTORY_SIZE;
    }
    if (input[strlen(input) - 1] == '\n') { // if last char in input is \n remove it
        input[strlen(input) - 1] = '\0';
    }
    history[*historyCounter] = strdup(input); // insert input into array at the appropriate index
    *historyCounter = (*historyCounter + 1) % MAX_HISTORY_SIZE; // increment to find next available position in history array
}

/**
 * Function prints the history in ascending order starting from 1.
 */
void printHistory(char *history[], int historyCounter, int historyHead) {
    int histNum = 1; // stores the ordering of the history
    int index = historyHead; // set the index to the head position of the history
    do {
        printf("%d %s\n", histNum, history[index]); // print history element
        histNum++;
        index = (index + 1) % MAX_HISTORY_SIZE; // calculate next postion in history array
    } while (index != historyCounter); 
}

/**
 * Function returns index of history command from history array on history invocation
 * returns -1 if failed
 * e.g 
 * !5 returns appropriate history array index
 * !-43 return -1
 */
int getHistoryCallIndex(char *history[], int historyCounter, char *tokens[], int historyHead) {
    int index;
    int historySize = getHistorySize(history);

    if (history[0] == NULL) { // print error if history is empty
        fprintf(stderr, "ERROR: history is empty\n");
        return -1;
    }
    else if (tokens[1] != NULL) { // print error if user gives "history" arguments
        fprintf(stderr, "%s: does not take any arguments\n", tokens[0]);
        return -1;
    }
    else if (strcmp(tokens[0], "!!") == 0) {
        index = (historyHead + (historySize - 1)) % MAX_HISTORY_SIZE; // calculate index for last element added to history
    }
    else if (isHistoryCmdValid(tokens[0]) == 0) { // check if history command is valid
        int x;
        sscanf(tokens[0], "!%d", &x); // read the int from the history invocation
        if (x < 0) {
            x = abs(x); // change negative to positve easier to read. a - (x) instead of a + (-x)
            if (x > historySize) {
                fprintf(stderr, "ERROR: the subtraction from history must be less than or equal to the history size: %d\n", historySize);
                return -1;
            }
            index = (historySize - (x % historySize) + historyCounter) % historySize;   // calculate index for circular array with subtraction from last index
        }
        else {
            if (x > 0 && x <= MAX_HISTORY_SIZE) { // check x is within 0 and the max number of elements in the history
                index = (historyHead + (x - 1)) % MAX_HISTORY_SIZE; // calculate index for circular array
            }
            else {
                index = x - 1; // set index to throw error if x is not within correct bounds
            }
        }
    }
    else { // print error when history invocation is not valid
        fprintf(stderr, "%s: not understood\n", tokens[0]);
        return -1;
    }

    if (index < 0 || index >= MAX_HISTORY_SIZE) {
        fprintf(stderr, "ERROR: history invocation must be between !1 and !%d\n", MAX_HISTORY_SIZE);
        return -1;
    }
    else if (history[index] == NULL) { // print error if the history element has not yet been filled
        fprintf(stderr, "ERROR: %s event not found\n", tokens[0]);
        return -1;
    }
    return index;
}

/**
 * Function checks if history invocation command is valid
 * Returns 0 if valid else 1
 */
int isHistoryCmdValid(char *str) {
    if (strlen(str) < 2) {
        return 1;
    }
    for (int i = 1; i < strlen(str); str++) { // start from the second character in the string as the first was already checked
        // if char is '-' and not at position 1 or if the char is not a digit return 1
        if (!((str[i] == '-' && i == 1) || (isdigit(str[i]) != 0))) {
            return 1;
        }
    }
    return 0;
}

/**
 * Function returns the number elements currently in the history array
 */
int getHistorySize(char *history[]) {
    int size = 0;
    while (size < MAX_HISTORY_SIZE && history[size] != NULL)
    {
        size++;
    }
    return size;
}

/**
 * Loads the history from the file ".hist_list"
 */
void loadHistory(char *history[], int *historyCounter, int *historyHead) {
    FILE *fptr;
    fptr = fopen(HISTORY_FILE_NAME, "r");

    // if file does not exist do nothing and return
    if (fptr == NULL) {
        return;
    }

    int historyCounterCopy = *historyCounter; // added variable copies so that value is kept at the end of the method
    int historyHeadCopy = *historyHead;

    if (fptr != NULL) {
        char line[MAX_INPUT_LENGTH];
        while (fgets(line, MAX_INPUT_LENGTH, fptr) != NULL) {
            if (line[strlen(line) - 1] == '\n') { // if last char in input is \n remove it
                line[strlen(line) - 1] = '\0';
            }
            addToHistory(line, history, &historyCounterCopy, &historyHeadCopy);
        }
    }

    *historyCounter = historyCounterCopy;
    *historyHead = historyHeadCopy;

    fclose(fptr);
}

/**
 * Saves the history in the file ".hist_list"
 */
void saveHistory(char *history[], int historyCounter, int historyHead) {

    // If history is empty do nothing
    if (history[0] == NULL) {
        return;
    }

    FILE *fptr = fopen(HISTORY_FILE_NAME, "w");
    if (fptr != NULL) {
        int histNum = 1;
        int index = historyHead;
        do {
            fprintf(fptr, "%s\n", history[index]);
            histNum++;
            index = (index + 1) % MAX_HISTORY_SIZE;
        } while (index != historyCounter);
    }
    fclose(fptr);
}

/**
 * Function initialises the key of the aliases to the null terminator
 */
void initialiseAliasKeys(alias aliasArray[MAX_ALIAS_SIZE]) {
    // aliasKey == "\0" indicates empty alias (no alias at index in aliasArray)
    for (int i = 0; i < MAX_ALIAS_SIZE; i++) {
        strcpy(aliasArray[i].aliasKey, "\0");
    }
}

/**
 * Function adds an alias
 */
void addAlias(char *tokens[], alias aliasArray[MAX_ALIAS_SIZE]) {

    if (tokens[2] == NULL) { // print error if alias commmands not provided
        fprintf(stderr, "ERROR: alias \"%s\" requires a command\n", tokens[1]);
        fprintf(stderr, "Suggestions: \"alias\" or \"alias <name> <command>\"\n");
    }
    else {
        // check if an alias already exits if so updated it and exit the method
        for (int i = 0; i < MAX_ALIAS_SIZE; i++) {
            if ((strcmp(aliasArray[i].aliasKey, "\0") != 0) && (strcmp(aliasArray[i].aliasKey, tokens[1]) == 0)) {
                strcpy(aliasArray[i].aliasKey, tokens[1]); // tokens[1] should be the alias key then commands from tokens[2]
                int tokensIndex = 2;
                int commandIndex = 0;
                while (tokens[tokensIndex] != NULL) {
                    strcpy(aliasArray[i].aliasCommands[commandIndex], tokens[tokensIndex]); // copy each token into the appropriate location
                    tokensIndex++;
                    commandIndex++;
                }
                aliasArray[i].noOfCommands = commandIndex;
                printf("alias: %s updated\n", aliasArray[i].aliasKey);
                return;
            }
        }

        // loop to find an available space for an alias if there is an available add the aliias and exit the method otherwise display error
        for (int i = 0; i < MAX_ALIAS_SIZE; i++) {
            if (strcmp(aliasArray[i].aliasKey, "\0") == 0) {
                strcpy(aliasArray[i].aliasKey, tokens[1]); // tokens[1] should be the alias key then commands from tokens[2]
                int tokensIndex = 2;
                int commandIndex = 0;
                while (tokens[tokensIndex] != NULL) {
                    strcpy(aliasArray[i].aliasCommands[commandIndex], tokens[tokensIndex]); // copy each token into the appropriate location
                    tokensIndex++;
                    commandIndex++;
                }
                aliasArray[i].noOfCommands = commandIndex;
                return;
            }
        }

        // print error if all availble aliases have been set
        fprintf(stderr, "ERROR: no more aliases can be set\n");
    }
}

/**
 * Function removes an alias
 */
void removeAlias(char *tokens[], alias aliasArray[MAX_ALIAS_SIZE]) {
    if (tokens[1] == NULL) { // print error if argument not provided to "unalias"
        fprintf(stderr, "unalias: requires one argument\n");
    }
    else if (tokens[2] == NULL) {

        // check if alias exists then set key to null terminator to reomve alias
        for (int i = 0; i < MAX_ALIAS_SIZE; i++) {
            if ((strcmp(aliasArray[i].aliasKey, "\0") != 0) && (strcmp(aliasArray[i].aliasKey, tokens[1]) == 0)) {
                strcpy(aliasArray[i].aliasKey, "\0");
                aliasArray[i].noOfCommands = 0;
                return;
            }
        }

        // print error if alias not found
        fprintf(stderr, "unalias: %s: not found\n", tokens[1]);
    }
    else { // print error if more than one argument provided to "unalias"
        fprintf(stderr, "unalias: requires only one argument\n");
    }
}

/**
 * Function prints all alias which have been set up
 */
void printAliases(alias aliasArray[MAX_ALIAS_SIZE]) {

    int aliasPrinted = 0;

    // search through the aliases if an alias exists print the alias
    for (int i = 0; i < MAX_ALIAS_SIZE; i++) {
        if (strcmp(aliasArray[i].aliasKey, "\0") != 0) {
            printf("alias %s='", aliasArray[i].aliasKey);
            for (int j = 0; j < aliasArray[i].noOfCommands; j++) {
                if (j == (aliasArray[i].noOfCommands - 1)) {
                    printf("%s", aliasArray[i].aliasCommands[j]);
                }
                else {
                    printf("%s ", aliasArray[i].aliasCommands[j]);
                }
            }
            printf("'\n");
            aliasPrinted = 1;
        }
    }

    // if no alias were printed print an error message
    if (aliasPrinted == 0) {
        fprintf(stderr, "ERROR: no aliases have been set up\n");
    }
}

/**
 * Function checks if a token is an alias
 * If the token is an alias the index of that alias in the alias array will be returned otherwise -1 will be returned
 */
int isAlias(char *token, alias aliasArray[MAX_ALIAS_SIZE]) {

    for (int i = 0; i < MAX_ALIAS_SIZE; i++) {
        // check alias key is not empty and check if the alias key is the same as the given token
        if ((strcmp(aliasArray[i].aliasKey, "\0") != 0) && (strcmp(aliasArray[i].aliasKey, token) == 0)) {
            return i;
        }
    }

    return -1;
}

/**
 * Function loads in alias from the ".aliases" file
 */
void loadAliases(alias aliasArray[MAX_ALIAS_SIZE]) {

    FILE *fptr;
    fptr = fopen(ALIAS_FILE_NAME, "r");

    // if file does not exist do nothing and return
    if (fptr == NULL) {
        return;
    }

    if (fptr != NULL) {
        char line[MAX_INPUT_LENGTH];
        char *token;
        int index = 0;
        int commandIndex = 0;
        while (fgets(line, MAX_INPUT_LENGTH, fptr) != NULL) {
            if (line[strlen(line) - 1] == '\n') { // if last char in input is \n remove it
                line[strlen(line) - 1] = '\0';
            }
            if (strcmp(line, "\0") != 0) { 
                token = strtok(line, " "); // tokenize the line appropriatly for storing the alias
                strcpy(aliasArray[index].aliasKey, token); // first token is alias key followed by commands
                commandIndex = 0;
                token = strtok(NULL, " ");
                while (token != NULL) {
                    strcpy(aliasArray[index].aliasCommands[commandIndex], token);
                    token = strtok(NULL, " ");
                    commandIndex++;
                }
                aliasArray[index].noOfCommands = commandIndex;
                index++;
            }
        }
    }
    fclose(fptr);
}

/**
 * Function saves alias into the ".aliases" file
 * File format:
 * <alias key> <command tokens>
 */
void saveAliases(alias aliasArray[MAX_ALIAS_SIZE]) {

    FILE *fptr = fopen(ALIAS_FILE_NAME, "w");

    if (fptr != NULL) {
        // check if an alias exists if it does it will be saved to the file
        for (int i = 0; i < MAX_ALIAS_SIZE; i++) {
            if (strcmp(aliasArray[i].aliasKey, "\0") != 0) {
                fprintf(fptr, "%s ", aliasArray[i].aliasKey);
                for (int j = 0; j < aliasArray[i].noOfCommands; j++) {
                    if (j == (aliasArray[i].noOfCommands - 1)) {
                        fprintf(fptr, "%s", aliasArray[i].aliasCommands[j]);
                    }
                    else {
                        fprintf(fptr, "%s ", aliasArray[i].aliasCommands[j]);
                    }
                }
                fprintf(fptr, "\n");
            }
        }
    }

    fclose(fptr);
}

/**
 * Function is used in the tokenization stage to check if tokens can still be replaced by an alias
 * 0 returned if an alias exists within the tokens else 1 is returned
 */
int checkAliasExistInTokens(char *tokens[], alias aliasArray[MAX_ALIAS_SIZE]){
    int index = 0;

    // loop through the tokens array and check if that token is an alias
    while (tokens[index] != NULL){
        if (isAlias(tokens[index], aliasArray) >= 0){
            return 0;
        }
        index++;
    }

    return 1;
}
