#include "gameLogic.h"

Player *create_player()
{
    Player *player = malloc(sizeof(Player));
    player->score = 0;
    return player;
}

Player *create_player_with_client(Client *client)
{
    Player *player = create_player();
    if (client != NULL)
        client->isPlaying = TRUE;
    return player;
}

Game *new_game(Player *player1, Player *player2)
{
    Game *game = malloc(sizeof(Game));

    game->players[0] = player1;
    game->players[1] = player2;

    // chooses randomly who starts
    srand(time(NULL));
    game->turn = rand() % 2 == 0 ? player1 : player2;

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            game->board[i][j] = 4;
        }
    }
    return game;
}

void free_player(Player *player)
{
    free(player);
}

void free_game(Game *game)
{
    free(game);
}

void print_board(Game *game)
{
    printf("\n\n\n");
    printf("    0 | 1 | 2 | 3 | 4 | 5 | Scores\n");
    printf("---------------------------------------------------\n");
    printf(RED "J1 | %d   %d   %d   %d   %d   %d | %d\n" RESET, game->board[0][0], game->board[0][1], game->board[0][2], game->board[0][3], game->board[0][4], game->board[0][5], game->players[0]->score);
    printf("---------------------------------------------------\n");
    printf("J2 | %d   %d   %d   %d   %d   %d | %d\n", game->board[1][0], game->board[1][1], game->board[1][2], game->board[1][3], game->board[1][4], game->board[1][5], game->players[1]->score);
    printf("---------------------------------------------------\n");
    printf("    11 | 10 | 9 | 8 | 7 | 6\n");
}

Player *get_current_player(Game *game)
{
    return game->turn;
}

Player *get_opponent(Player *player, Game *game)
{
    if (game->players[0] == player)
    {
        return game->players[1];
    }
    else
    {
        return game->players[0];
    }
}

int get_seeds(Pit pit, Game *game)
{
    return game->board[pit.line][pit.column];
}

void empty_seeds(Game *game, Player *player)
{
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            player->score += game->board[i][j];
            game->board[i][j] = 0;
        }
    }
}

// TODO : saves the opponent from starving if you can
boolean is_valid_move(Pit pit, Game *game)
{
    if (pit.line < 0 || pit.line > 1 || pit.column < 0 || pit.column > 5 || get_seeds(pit, game) == 0 || (game->turn == game->players[0] && pit.line == 1) || (game->turn == game->players[1] && pit.line == 0))
    {
        return FALSE;
    }
    return TRUE;
}

void make_move(Pit pit, Game *game)
{
    int seeds = get_seeds(pit, game);
    game->board[pit.line][pit.column] = 0;
    int line = pit.line;
    int column = pit.column;
    while (seeds > 0)
    {
        if (line == 0)
        {
            column++;
            if (column == pit.column && line == pit.line)
            {
                column++;
            }
            if (column > 5)
            {
                line = 1;
                column = 5;
            }
        }
        else
        {
            column--;
            if (column == pit.column && line == pit.line)
            {
                column--;
            }
            if (column < 0)
            {
                line = 0;
                column = 0;
            }
        }
        game->board[line][column]++;
        seeds--;
    }
    // TODO : implement logic of capture and starvation
    game->turn = get_opponent(game->turn, game);
}

Player *player_line_empty(Game *game)
{
    for (int i = 0; i < 2; i++)
    {
        boolean lineEmpty = TRUE;
        for (int j = 0; j < 6; j++)
        {
            if (game->board[i][j] > 0)
            {
                lineEmpty = FALSE;
            }
        }
        if (lineEmpty)
        {
            return game->players[i];
        }
    }
    return NULL;
}

boolean is_game_over(Game *game)
{
    Player *player;
    // if one player has above 25 captured seeds
    if (game->players[0]->score >= 25 || game->players[1]->score >= 25)
    {
        return TRUE;
    }
    // if one player has no seeds left on his line and the next player can't feed him
    else if ((player = player_line_empty(game)) != NULL)
    {
        if (check_starvation(game, player))
        {
            empty_seeds(game, get_opponent(player, game));
            return TRUE;
        }
    }
    // if it is not possible to capture seeds
    // TODO : change check_capture call
    // else if (check_capture(game))
    // {
    //     return TRUE;
    // }
    return FALSE;
}

boolean check_starvation(Game *game, Player *player)
{
    if (game->players[0] == player)
    {
        for (int j = 0; j < 6; j++)
        {
            if (game->board[0][j] >= 6 - j)
            {
                return TRUE;
            }
        }
    }
    else
    {
        for (int j = 5; j >= 0; j--)
        {
            if (game->board[1][j] >= j + 1)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

// TODO
void capture(Game *game, Pit startingPit)
{
    boolean capture = FALSE;
    int currentLine = startingPit.line;
    int currentColumn = startingPit.column;
    int seedsCollected = 0;

    // Check if the starting pit is empty or has more than 3 seeds
    if (game->board[currentLine][currentColumn] != 2 || game->board[currentLine][currentColumn] != 3)
        return;

    // Collect seeds and look at previous squares until conditions met
    while (game->board[currentLine][currentColumn] == 2 || game->board[currentLine][currentColumn] == 3)
    {
        seedsCollected += game->board[currentLine][currentColumn];
        game->board[currentLine][currentColumn] = 0;

        // Move to the previous column in the opposing camp
        if (currentLine == 0 && currentColumn > 0)
        {
            currentColumn--; // Move left on player 0's camp
        }
        else if (currentLine == 1 && currentColumn < 6)
        {
            currentColumn++; // Move right on player 1's camp
        }
        else
        {
            return;
        }
    }

    // Check if the opponent has seeds left in their camp
    boolean opponentHasSeeds = FALSE;
    for (int j = 0; j < 6; j++)
    {
        if (game->board[currentLine][j] > 0)
        {
            opponentHasSeeds = TRUE;
            break;
        }
    }

    // If the opponent has seeds, the capture is valid
    if (opponentHasSeeds)
    {
        get_current_player(game)->score = seedsCollected;
    }
}

Player *get_winner(Game *game)
{
    if (game->board[0][5] > game->board[1][5])
    {
        return game->players[0];
    }
    else if (game->board[0][5] < game->board[1][5])
    {
        return game->players[1];
    }
    else
    {
        return NULL;
    }
}

int main()
{
    Player *player1 = create_player();
    Player *player2 = create_player();
    Game *game = new_game(player1, player2);
    while (!is_game_over(game))
    {
        print_board(game);
        Pit pit;
        int caseNumber;
        printf("\nJoueur %d, entrez le numéro de la case à jouer : ", game->turn == player1 ? 1 : 2);
        scanf("%d", &caseNumber);
        if (caseNumber < 0 || caseNumber > 11)
        {
            printf("Mouvement invalide\n");
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
            make_move(pit, game);
        }
        else
        {
            printf("Mouvement invalide\n");
        }
    }
}
