#ifndef IO_H
#define IO_H

#include "result.h"
#include "util.h"

Result readFile(String path, String* output);
String input(void);
void printHelp(bool detailed);

#define STYLE(text, color, style) "\033[" style ";3" color "m" text "\033[0m"
#define COLOR(text, color) STYLE(text, color, NORMAL)
#define FORMAT(text, style) "\033[" style "m" text "\033[0m"

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

#endif
