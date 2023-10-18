#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "clientServer.h"

#define TRUE 1
#define FALSE 0
typedef int boolean;

#define RED "\033[0;31m"
#define RESET "\033[0m"

typedef struct
{
    int score;
} Player;

typedef struct Game
{
    Player *players[2];
    Player *turn;
    int board[2][6];
} Game;

typedef struct
{
    int line;
    int column;
} Pit;

// Initializes a new player
Player *create_player();

// Initializes a new player of the given client
Player *create_player_with_client(Client *client);

// Initializes a new game with the given player names and returns a pointer to the game.
Game *new_game(Player *player1, Player *player2);

// Frees the memory allocated for the given player.
void free_player(Player *player);

// Frees the memory allocated for the given game.
void free_game(Game *game);

// Prints the game board on stdout.
void print_board(Game *game);

// Returns the current player whose turn it is.
Player *get_current_player(Game *game);

// Returns the opponent of the given player.
Player *get_opponent(Player *player, Game *game);

// Returns the number of seeds in the given pit.
int get_seeds(Pit pit, Game *game);

// Give all remaining seeds to the player.
void empty_seeds(Game *game, Player *player);

// Returns true if the given move is valid for the current player, false otherwise.
boolean is_valid_move(Pit pit, Game *game);

// Executes the given move for the current player and returns the number of seeds captured.
void make_move(Pit pit, Game *game);

// Returns the player whose line is empty if there is, null otherwise.
Player *player_line_empty(Game *game);

// Returns true if the game is over, false otherwise.
boolean is_game_over(Game *game);

// Returns true if the given player can't be fed, false otherwise.
boolean check_starvation(Game *game, Player *player);

// Returns true if player can capture, false otherwise.
void capture(Game *game, Pit pit);

// Returns the winner of the game, or NULL if the game is not over or it is a tie.
Player *get_winner(Game *game);

#endif // GAME_LOGIC_H
