#include "display.h"

void print_board(Game *game)
{
    printf("\n\n\n");
    printf("Direction " RED "--->\n" RESET);
    printf("Case    |  1   2   3   4   5   6  | Scores\n");
    printf("---------------------------------------------------\n");
    printf(GREEN "%s J1  | %2d  %2d  %2d  %2d  %2d  %2d  | %d\n" RESET, game->turn == game->players[0] ? RED "-->" GREEN : "   ", game->board[0][0], game->board[0][1], game->board[0][2], game->board[0][3], game->board[0][4], game->board[0][5], game->players[0]->score);
    printf("---------------------------------------------------\n");
    printf(PURPLE "%s J2  | %2d  %2d  %2d  %2d  %2d  %2d  | %d\n" RESET, game->turn == game->players[1] ? RED "-->" PURPLE : "   ", game->board[1][0], game->board[1][1], game->board[1][2], game->board[1][3], game->board[1][4], game->board[1][5], game->players[1]->score);
    printf("---------------------------------------------------\n");
    printf("          12  11  10   9   8   7\n");
}

void construct_board(Game *game, char *board)
{
    sprintf(board, "\n\n\n");
    sprintf(board, "%s\t\tDirection " CYAN "--->\n\n" RESET, board);
    sprintf(board, "%sCase    |  1   2   3   4   5   6  | Scores\n", board);
    sprintf(board, "%s---------------------------------------------------\n", board);
    sprintf(board, "%s" GREEN "%s J1  | %2d  %2d  %2d  %2d  %2d  %2d  | %d\n" RESET, board, game->turn == game->players[0] ? RED "-->" GREEN : "   ", game->board[0][0], game->board[0][1], game->board[0][2], game->board[0][3], game->board[0][4], game->board[0][5], game->players[0]->score);
    sprintf(board, "%s---------------------------------------------------\n", board);
    sprintf(board, "%s" PURPLE "%s J2  | %2d  %2d  %2d  %2d  %2d  %2d  | %d\n" RESET, board, game->turn == game->players[1] ? RED "-->" PURPLE : "   ", game->board[1][0], game->board[1][1], game->board[1][2], game->board[1][3], game->board[1][4], game->board[1][5], game->players[1]->score);
    sprintf(board, "%s---------------------------------------------------\n", board);
    sprintf(board, "%s          12  11  10   9   8   7\n", board);
}

void construct_turn_message(Game *game, char *message)
{
    sprintf(message, "It's player %d's turn !\n", game->turn == game->players[0] ? 1 : 2);
}