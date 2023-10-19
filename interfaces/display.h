#ifndef DISPLAY_H
#define DISPLAY_H

#define RED "\033[0;31m"
#define RESET "\033[0m"

#include "gameLogic.h"
#include <stdio.h>

// Prints the game board on stdout.
void print_board(Game *game);

#endif
