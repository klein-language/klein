#ifndef IO_H
#define IO_H

#include "result.h"

bool fileExists(String path);
KleinResult readFile(String path, String* output);
String input(void);
void printHelp(bool detailed);

#define STYLE(text, color, style) "\033[" style ";3" color "m" text "\033[0m"
#define COLOR(text, color) STYLE(text, color, NORMAL)
#define DECORATE(text, style) "\033[" style "m" text "\033[0m"

#define RED "1"
#define GREEN "2"
#define YELLOW "3"
#define BLUE "4"
#define PURPLE "5"
#define CYAN "6"
#define WHITE "7"

#define NORMAL "0"
#define BOLD "1"
#define UNDERLINE "4"

#define HANDLE_COMMAND_CONFLICTS(command, action, numberOfArguments, arguments)                                                                                          \
	if (fileExists(command)) {                                                                                                                                           \
		fprintf(stderr, "\n%s %s refers to both a klein command and a file in the current directory.\n\n", STYLE("Warning:", YELLOW, BOLD), STYLE(command, BLUE, BOLD)); \
		fprintf(stderr, "Would you like to:\n");                                                                                                                         \
		fprintf(stderr, "    1. Execute the built-in command %s %s\n", STYLE("klein", PURPLE, BOLD), STYLE(command, BLUE, BOLD));                                        \
		fprintf(stderr, "    2. Run the file called %s (%s %s %s)\n", COLOR(command, RED), STYLE("klein", PURPLE, BOLD), STYLE("run", BLUE, BOLD), COLOR(command, RED)); \
		fprintf(stderr, "Enter 1 or 2: ");                                                                                                                               \
		String choice = input();                                                                                                                                         \
		if (strcmp(choice, "1") == 0) {                                                                                                                                  \
			action;                                                                                                                                                      \
		} else {                                                                                                                                                         \
			runFile(numberOfArguments, arguments);                                                                                                                       \
		}                                                                                                                                                                \
	} else {                                                                                                                                                             \
		action;                                                                                                                                                          \
	}

#endif
