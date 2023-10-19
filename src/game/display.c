#include "display.h"

void print_board(Game *game)
{
    printf("\n\n\n");
    printf("Case |  0   1   2   3   4   5  | Scores\n");
    printf("---------------------------------------------------\n");
    printf(RED " J1  | %2d  %2d  %2d  %2d  %2d  %2d  | %d\n" RESET, game->board[0][0], game->board[0][1], game->board[0][2], game->board[0][3], game->board[0][4], game->board[0][5], game->players[0]->score);
    printf("---------------------------------------------------\n");
    printf(" J2  | %2d  %2d  %2d  %2d  %2d  %2d  | %d\n", game->board[1][0], game->board[1][1], game->board[1][2], game->board[1][3], game->board[1][4], game->board[1][5], game->players[1]->score);
    printf("---------------------------------------------------\n");
    printf("       11  10   9   8   7   6\n");
}