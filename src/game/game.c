#include "display.h"
#include "gameLogic.h"

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

// Play the game
Game *playGame(Game *game)
{
    while (!is_game_over(game))
    {
        print_board(game);
        Pit pit;
        int caseNumber;
        printf("\nPlayer %d, enter the pit (< 0 or > 11 == tie) in which you want to play : ", game->turn == game->players[0] ? 1 : 2);
        scanf("%d", &caseNumber);
        if (caseNumber < 0 || caseNumber > 11)
        {
            tie(game);
            game->turn = get_opponent(game->turn, game);
            continue;
        }
        else if (caseNumber <= 5)
        {
            pit.line = 0;
            pit.column = caseNumber;
        }
        else if (caseNumber <= 11)
        {
            pit.line = 1;
            pit.column = 11 - caseNumber;
        }
        if (is_valid_move(pit, game))
        {
            game = make_move(pit, game);
        }
        else
        {
            printf("Invalid move\n");
        }
    }
    printf("\n\n\n\n\n Final board !\n");
    print_board(game);
    Player *winner = get_winner(game);
    if (winner != NULL)
    {
        printf("Player %d won !\n", winner == game->players[0] ? 1 : 2);
    }
    else
    {
        printf("Tie !\n");
    }
    return game;
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

    game = playGame(game);

    free_game(game);
    free_player(player1);
    free_player(player2);

    return 0;
}
