#include "../include/io.h"
#include "../include/result.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>

Result readFile(String path, String* output) {

	// Open file
	FILE* file = fopen(path, "rb");
	ASSERT_NONNULL(file);

	// Get file size
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate space
	char* buffer = malloc((unsigned long) length + 1);
	ASSERT_NONNULL(buffer);

	// Read & close file
	fread(buffer, 1, (unsigned long) length, file);
	fclose(file);
	buffer[length] = '\0';

	RETURN_OK(output, buffer);
}

void printHelp(bool detailed) {
	if (!detailed) {
		fprintf(stderr, "\n %s\n\n", STYLE("Klein", CYAN, BOLD));
		fprintf(stderr, " %s\n\n", "Poetry in programming.");
		fprintf(stderr, " %s: %s %s %s %s\n\n", FORMAT("Usage", UNDERLINE), STYLE("klein", PURPLE, BOLD), STYLE("<COMMAND>", BLUE, BOLD), COLOR("[OPTIONS]", YELLOW), COLOR("[ARGUMENTS]", RED));

		// Commands
		fprintf(stderr, " %s\n", STYLE("Commands:", CYAN, BOLD));
		fprintf(stderr, " \t%s %s               Run a klein file\n", STYLE("run", BLUE, BOLD), COLOR("<FILE>", RED));
		fprintf(stderr, " \t%s %s             Check a klein file for static errors\n", STYLE("check", BLUE, BOLD), COLOR("<FILE>", RED));
		fprintf(stderr, " \t%s                  Print version information\n", STYLE("version", BLUE, BOLD));
		fprintf(stderr, " \t%s %s        Show the help menu\n\n", STYLE("help", BLUE, BOLD), COLOR("[--detailed]", YELLOW));
	}
}

String input(void) {
	String buffer;
	size_t length = 0;
	getline(&buffer, &length, stdin);
	return buffer;
}
