// skalman.c -- a bad stupid Unix shell
// Thomas Padron-McCarthy (thomas.padron-mccarthy@oru.se)
// Thu Apr 25 11:42:55 CEST 2019

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 1000
#define MAX_WORD_LENGTH 100
#define MAX_WORDS 100

// Splits "Hi,   Ho! Silver!!!   " into "Hi,", "Ho!" and "Silver!!!",
// Returns the number of words in the result (3 in the example above).
int split_line(char *line, char words[][MAX_WORD_LENGTH + 1]) {
    int linepos = 0, wordnr = 0;
    while (line[linepos] != '\0' && isspace(line[linepos]))
        ++linepos;
    while (line[linepos] != '\0') {
        if (wordnr >= MAX_WORDS) {
            printf("skalman: Warning - too many words on line. Max is %d.\n", MAX_WORDS);
            break;
        }
        int wordpos = 0;
        while(line[linepos] != '\0' && !isspace(line[linepos])) {
            if (wordpos >= MAX_WORD_LENGTH) {
	      printf("skalman: Warning - word too long. Max length is %d.\n", MAX_WORD_LENGTH);
                break;
            }
            words[wordnr][wordpos++] = line[linepos++];
        }
        words[wordnr][wordpos] = '\0';
        ++wordnr;
        while (line[linepos] != '\0' && isspace(line[linepos]))
            ++linepos;
    }
    words[wordnr][0] = '\0';
    return wordnr;
} // split_line

// Executes a command that has been split into an array of words
void handle_command(char words[][MAX_WORD_LENGTH + 1], int nr_words) {
    if (strcmp(words[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(words[0], "chdir") == 0 || strcmp(words[0], "cd") == 0) {
        if (nr_words != 2)
            printf("skalman: chdir takes exactly one argument\n");
        else {
            if (chdir(words[1]) != 0)
                printf("skalman: Couldn't chdir to '%s'. Error %d = %s\n", words[1], errno, strerror(errno));
	    else {
	      printf("skalman: changed directory to '%s'\n", words[1]);
	    }
        }
    }
    else {

        // Start a new process, start the given command, wait for it to finish!
        // Perhaps useful hint: Does the array "words" have the right structure for execv et al.?
        // A shell should give useful feedback, so don't forget error checking!

        // For extra points: Handle I/O redirect with "<" and ">"

      pid_t child_pid;
      int child_status;

      //FILE *fr = fopen(argv[1], "r");

       //fclose(fr);

      
      child_pid = fork();
      if(child_pid == 0) {
	/* This is done by the child process. */
	char *argv[nr_words];
	for(int i = 0; i <= nr_words; i++) {
	  argv[i] = words[i];
	}
	argv[nr_words] = NULL;

	// Only used so that ls can be used by itself
	if(argv[1] == NULL) {
	  execvp(argv[0], argv);
	}

	// If second argument is >, writes to the file right of it
	if(strcmp(argv[1], (char*)">") == 0){
	  fclose(stdout);
	  
	  FILE *fw = fopen(argv[2], "w");

	  argv[1] = NULL;
	  execvp(argv[0], argv);
	}
	
	execvp(argv[0], argv);

    
	/* If execvp returns, it must have failed. */

	printf("skalman: Unknown command\n");
	exit(0);
      } else {
	/* This is run by the parent.  Wait for the child
        to terminate. */
        wait(&child_status);

      }
      
    }
} // handle_command

int main(void) {
    printf("Skalman shell starting. Exit with exit or EOF.\n");
    char command_line[MAX_LINE_LENGTH + 1 + 1]; // +2 for '\n' and '\0'
    char command_words[MAX_WORDS + 1][MAX_WORD_LENGTH + 1]; // +1 for NULL and +1 for '\0'
    while (printf("skalman> "), fflush(stdout), fgets(command_line, sizeof command_line, stdin) != NULL) {
        if (strlen(command_line) == MAX_LINE_LENGTH + 1 && command_line[MAX_LINE_LENGTH] != '\n' && !feof(stdin))
            printf("skalman: Warning - line too long. Max length is %d.\n", MAX_LINE_LENGTH);
        int nr_words = split_line(command_line, command_words);
        if (nr_words == 0)
            printf("skalman: Empty command line.\n");
        else {

            // Some output to simplify debugging
            printf("skalman: Command has %d words: [ ", nr_words);
            for (int i = 0; i < nr_words; ++i) {
                printf("'%s'", command_words[i]);
                if (i < nr_words - 1)
                    printf(", ");
            }
            printf(" ]\n");

            handle_command(command_words, nr_words);
        }
    }
    return EXIT_SUCCESS;
} // main
