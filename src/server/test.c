#include "gameLogic.h"
#include <stdio.h>

// Fill in manually the board of the game
void manual_fill_board(Game *game)
{
    printf("Manual Board Initialization\n");

    for (int line = 0; line < 2; line++)
    {
        for (int column = 0; column < 6; column++)
        {
            printf("Enter the number of seeds for player %d in pit %d, %d: ", line + 1, line, column);
            int numSeeds;
            scanf("%d", &numSeeds);
            game->board[line][column] = numSeeds;
        }
    }
}

int main()
{
    Player *player1 = create_player();
    Player *player2 = create_player();

    Game *game = new_game(player1, player2);

    int InitChoice;

    printf("\n\nHow would you like to initialize the game board?\n");
    printf("1. Normal Initialization\n");
    printf("2. Manual Initialization\n");
    printf("Enter your choice (1 or 2): ");
    scanf("%d", &InitChoice);

    if (InitChoice == 2)
    {
        manual_fill_board(game);
    }

    playGame(game);
    
    return 0;
}
