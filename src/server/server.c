#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "display.h"
#include <pthread.h>
#include <signal.h>

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

Client *clients[MAX_CLIENTS];
pthread_t threads[MAX_THREADS];

void *clientHandler(void *indexInClients)
{
   int index = *(int *)indexInClients;
   Client *client = clients[index];

   fd_set rdfs;
   FD_ZERO(&rdfs);
   FD_SET(client->sock, &rdfs);

   while (1)
   {
      handleGame(client);
      if (FD_ISSET(client->sock, &rdfs))
      {
         if (handleMenu(client) == SOCKET_ERROR)
         {
            break;
         }
      }
   }

   printf("Client %s disconnected\n", client->name);
   clear_client(index);
   return NULL;
}

static boolean check_socket(int sockFd, char *tempBuffer)
{
   fd_set read_fds;
   struct timeval tv;
   FD_ZERO(&read_fds);
   FD_SET(sockFd, &read_fds);

   tv.tv_sec = 1;
   tv.tv_usec = 0;

   int select_ret = select(sockFd + 1, &read_fds, NULL, NULL, &tv);

   if (select_ret == -1)
   {
      perror("select");
      return FALSE;
   }
   else if (select_ret > 0)
   {
      // Data available, check if disconnect
      int bytes_read = read_client(sockFd, tempBuffer);
      if (bytes_read <= 0)
      {
         return FALSE;
      }
   }
   return TRUE;
}

static int add_client(Client *client)
{
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] == NULL)
      {
         clients[i] = client;
         return i;
      }
      else if (strcmp(clients[i]->name, client->name) == 0)
      {
         return USERNAME_TAKEN_ERROR;
      }
      else if (strlen(client->name) == 0)
      {
         return USERNAME_EMPTY_ERROR;
      }
   }
   return MAX_PLAYERS_ERROR;
}

static void list_commands(Client *client)
{
   char buffer[BUF_SIZE];
   buffer[0] = 0;
   strncat(buffer, "\nList of commands:\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "list" RESET " : list the available players\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "bio" RESET " : edit your bio\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "consult" RESET " : consult the bio of another player\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "friends" RESET " : manage your friends\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "observe" RESET " : observe a game\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "challenge" RESET " : challenge a player\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "accept" RESET " : accept a challenge\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "refuse" RESET " : refuse a challenge\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "quit" RESET " : quit the game\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " GREEN "any other command will be sent as a message chat\n" RESET, BUF_SIZE - strlen(buffer) - 1);
   write_client(client->sock, buffer);
}

static void list_clients(char *buffer, Client *client)
{
   buffer[0] = 0;
   strncat(buffer, "\nList of clients:\n", BUF_SIZE - strlen(buffer) - 1);
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL)
      {
         strncat(buffer, "\t- ", BUF_SIZE - strlen(buffer) - 1);
         if (clients[i] == client)
         {
            strncat(buffer, GREEN, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, clients[i]->name, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, " (you)", BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, RESET, BUF_SIZE - strlen(buffer) - 1);
         }
         else
         {
            strncat(buffer, clients[i]->name, BUF_SIZE - strlen(buffer) - 1);
            if (clients[i]->game != NULL)
               strncat(buffer, " (in game)", BUF_SIZE - strlen(buffer) - 1);
         }
         strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
      }
   }
}

static void list_clients_not_in_game(char *buffer, Client *client)
{
   buffer[0] = 0;
   strncat(buffer, "\nList of clients:\n", BUF_SIZE - strlen(buffer) - 1);
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL && clients[i]->game == NULL && clients[i]->challengedBy == NULL)
      {
         strncat(buffer, "\t- ", BUF_SIZE - strlen(buffer) - 1);
         if (clients[i] == client)
         {
            strncat(buffer, GREEN, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, clients[i]->name, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, " (you)", BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, RESET, BUF_SIZE - strlen(buffer) - 1);
         }
         else
         {

            strncat(buffer, clients[i]->name, BUF_SIZE - strlen(buffer) - 1);
         }
         strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
      }
   }
}

static boolean list_not_friends(char *buffer, Client *client)
{
   buffer[0] = 0;
   boolean foundOne = FALSE;
   strncat(buffer, "\nList of clients not friends with you:\n", BUF_SIZE - strlen(buffer) - 1);
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL)
      {
         if (clients[i] == client)
         {
            continue;
         }
         boolean isFriend = FALSE;
         for (int j = 0; j < MAX_FRIENDS; j++)
         {
            if (client->friends[j] == clients[i])
            {
               isFriend = TRUE;
               break;
            }
         }
         if (!isFriend)
         {
            foundOne = TRUE;
            strncat(buffer, "\t- ", BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, clients[i]->name, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
         }
      }
   }
   if (!foundOne)
   {
      strncat(buffer, RED "None\n" RESET, BUF_SIZE - strlen(buffer) - 1);
   }
   return foundOne;
}

static boolean list_friends(char *buffer, Client *client)
{
   buffer[0] = 0;
   boolean foundOne = FALSE;
   strncat(buffer, "\nList of friends:\n", BUF_SIZE - strlen(buffer) - 1);
   for (int i = 0; i < MAX_FRIENDS; i++)
   {
      if (client->friends[i] != NULL)
      {
         foundOne = TRUE;
         strncat(buffer, "\t- ", BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, client->friends[i]->name, BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
      }
   }
   if (!foundOne)
   {
      strncat(buffer, RED "None\n" RESET, BUF_SIZE - strlen(buffer) - 1);
   }
   return foundOne;
}

static boolean list_games(char *buffer, Client *client)
{
   buffer[0] = 0;
   boolean foundOne = FALSE;
   strncat(buffer, "\nList of games:\n", BUF_SIZE - strlen(buffer) - 1);
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL && clients[i]->game != NULL && clients[i]->player == clients[i]->game->players[0])
      {
         if (canObserve(clients[i], client))
         {
            foundOne = TRUE;
            strncat(buffer, "\t- ", BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, clients[i]->name, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, " (P1)" RED " VS " RESET, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, clients[i]->challengedBy->name, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, " (P2)\n", BUF_SIZE - strlen(buffer) - 1);
         }
      }
   }
   if (!foundOne)
   {
      strncat(buffer, RED "No game available\n" RESET, BUF_SIZE - strlen(buffer) - 1);
   }
   return foundOne;
}

static Client *getClientByName(const char *name)
{
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL && strcmp(clients[i]->name, name) == 0)
      {
         return clients[i];
      }
   }
   return NULL;
}

static void handleGame(Client *client)
{
   while (client->game != NULL)
   {
      Game *game = client->game;
      char buffer[BUF_SIZE];
      boolean quit = FALSE;
      while (game != NULL && game->turn != NULL && game->turn == client->challengedBy->player)
      {
         char tempBuffer[BUF_SIZE];
         tempBuffer[0] = 0;
         char message[BUF_SIZE];
         message[0] = 0;

         // Checks for disconnect and absorbs all inputs while waiting for the other player
         if (!check_socket(client->sock, tempBuffer))
         {
            quit = TRUE;
            break;
         }
         else if (strlen(tempBuffer) > 0)
         {
            send_chat_message_to_all_observers(client, tempBuffer, client->player == game->players[0] ? GREEN : PURPLE);
         }
      }
      // Pointer to game may have changed in the other thread
      // Checks if the game is still going
      // Checks for disconnect
      if ((game = client->game) == NULL || game->turn == NULL || quit)
      {
         break;
      }

      write_client(client->sock, "\n" RED "It's your turn !\n" RESET "Type " PURPLE "tie" RESET " to tie\n" BLUE "Type anything else to send it to your opponent and the observers\n" RESET);
      char *nameP1 = client->player == game->players[0] ? client->name : client->challengedBy->name;
      char *nameP2 = client->player == game->players[1] ? client->name : client->challengedBy->name;
      construct_board(game, buffer, nameP1, nameP2);

      send_message_to_all_observers(game, buffer, NULL);

      // Check if the game and the socket are still alive
      if (client->game == NULL || read_client(client->sock, buffer) <= 0)
      {
         break;
      }
      else
      {
         game = client->game;
         int caseNumber = atoi(buffer);
         Pit pit;

         // Check if the player wants to tie
         if (strcmp(buffer, "tie") == 0)
         {
            tie(game);
            write_client(client->sock, GREEN "\nYou asked to tie !\n" RESET);
            write_client(client->challengedBy->sock, GREEN "\nYour opponent asked to tie !\n" RESET);
            game->turn = get_opponent(game->turn, game);
         }
         // Check if conversion was successful
         else if (caseNumber == 0)
         {
            send_chat_message_to_all_observers(client, buffer, client->player == game->players[0] ? GREEN : PURPLE);
         }
         else if (get_pit(caseNumber, &pit) && is_valid_move(pit, game))
         {
            make_move(&(client->game), pit);

            update_game_of_all_observers(game, client->game);
            game = client->game;

            write_client(client->sock, "\nMove done !\nWait for your turn...\nAny message " BLUE "typed will be sent to your opponent" RESET " and the observers.\n");
         }
         else
         {
            write_client(client->sock, RED "Invalid move\nPlease retry another one.\n" RESET);
         }

         // Game is over
         if (game != NULL && is_game_over(game))
         {
            construct_board(game, buffer, nameP1, nameP2);

            send_message_to_all_observers(game, buffer, NULL);

            Player *winner = get_winner(game);
            if (winner != NULL)
            {
               char message[BUF_SIZE];
               snprintf(message, BUF_SIZE, BLUE "\nPlayer %d won !\n" RESET, winner == game->players[0] ? 1 : 2);

               send_message_to_all_observers(game, message, NULL);

               if (winner == client->player)
               {
                  write_client(client->sock, GREEN "\nCongratulations, you won !\n" RESET);
                  write_client(client->challengedBy->sock, RED "\nYou lost !\n" RESET);
               }
               else
               {
                  write_client(client->sock, RED "\nYou lost !\n" RESET);
                  write_client(client->challengedBy->sock, GREEN "\nCongratulations, you won !\n" RESET);
               }
            }
            else
            {
               send_message_to_all_observers(game, BLUE "\nIt's a tie !\n" RESET, NULL);
            }

            update_game_of_all_observers(game, NULL);

            // Free all the memory allocated for the game
            free_player(game->players[0]);
            free_player(game->players[1]);
            free_game(game);

            client->player = NULL;
            client->challengedBy->player = NULL;
            client->challengedBy->challengedBy = NULL;
            client->challengedBy = NULL;
            break;
         }
      }
   }
}

static int handleMenu(Client *client)
{
   char buffer[BUF_SIZE];
   list_commands(client);
   int c = read_client(client->sock, buffer);
   if (c <= 0)
   {
      return SOCKET_ERROR;
   }
   else
   {
      if (strcmp(buffer, "list") == 0)
      {
         list_clients(buffer, client);
         write_client(client->sock, buffer);
      }
      else if (strcmp(buffer, "bio") == 0)
      {
         if (handleBio(client) == SOCKET_ERROR)
            return SOCKET_ERROR;
      }
      else if (strcmp(buffer, "consult") == 0)
      {
         if (handleConsult(client) == SOCKET_ERROR)
            return SOCKET_ERROR;
      }
      else if (strcmp(buffer, "friends") == 0)
      {
         if (handleFriends(client) == SOCKET_ERROR)
            return SOCKET_ERROR;
      }
      else if (strcmp(buffer, "observe") == 0)
      {
         if (handleObserver(client) == SOCKET_ERROR)
            return SOCKET_ERROR;
      }
      else if (strcmp(buffer, "challenge") == 0)
      {
         if (challengeClient(client) == SOCKET_ERROR)
            return SOCKET_ERROR;
      }
      else if (strcmp(buffer, "accept") == 0)
      {
         if (client->challengedBy == NULL)
         {
            write_client(client->sock, "\nYou don't have any challenge to accept !\n");
         }
         else
         {
            write_client(client->sock, "\nYou accepted the challenge !\n" GREEN " Creating the game...\n" RESET);

            Player *p1 = create_player();
            Player *p2 = create_player();
            Game *game = new_game(p1, p2);
            client->game = game;
            client->challengedBy->game = game;
            client->player = p1;
            client->challengedBy->player = p2;

            char buffer[BUF_SIZE];
            snprintf(buffer, BUF_SIZE, "\nGame created between P1 : %s and P2 : %s !\n", client->name, client->challengedBy->name);
            write_client(client->sock, buffer);
            write_client(client->challengedBy->sock, buffer);
         }
      }
      else if (strcmp(buffer, "quit") == 0)
      {
         return SOCKET_ERROR;
      }
      else if (strcmp(buffer, "refuse") == 0)
      {
         if (client->challengedBy != NULL && client->challengedBy->challengedBy != NULL)
         {
            char message[BUF_SIZE];
            snprintf(message, BUF_SIZE, "\n%s refused your challenge !\n", client->name);
            write_client(client->challengedBy->sock, message);

            message[0] = 0;
            snprintf(message, BUF_SIZE, "\nYou refused %s's challenge !\n", client->challengedBy->name);
            write_client(client->sock, message);

            client->challengedBy->challengedBy = NULL;
            client->challengedBy = NULL;
         }
         else
         {
            write_client(client->sock, "\nYou don't have any challenge to refuse !\n");
         }
      }
      else
      {
         send_message_to_all_clients(client, buffer, 0);
      }
   }
   return EXIT_SUCCESS;
}

static int handleObserver(Client *client)
{
   char buffer[BUF_SIZE];
   while (1)
   {
      if (!list_games(buffer, client))
      {
         write_client(client->sock, "\nThere is no game available to observe !\n");
         break;
      }
      write_client(client->sock, "\nWhose game do you want to observe ?\n" RED "cancel" RESET " to cancel\n");
      write_client(client->sock, buffer);

      buffer[0] = 0;
      if (read_client(client->sock, buffer) <= 0)
         return SOCKET_ERROR;
      if (strcmp(buffer, "cancel") == 0)
      {
         break;
      }
      Client *observed = getClientByName(buffer);

      char message[BUF_SIZE];
      if (observed == NULL)
      {
         snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " not found !\n", buffer);
         write_client(client->sock, message);
      }
      else if (observed->game == NULL)
      {
         snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " is not playing !\n", buffer);
         write_client(client->sock, message);
      }
      else
      {
         // Check if client can watch observed's game
         if (!canObserve(observed, client))
         {
            snprintf(message, BUF_SIZE, "\nYou can't observe %s's game !\n", buffer);
            write_client(client->sock, message);
            break;
         }

         client->game = observed->game;
         snprintf(message, BUF_SIZE, "\nYou are now observing %s's game !\nType " RED "quit" RESET " to quit", observed->name);
         write_client(client->sock, message);

         while (client->game != NULL)
         {
            // Check for disconnect while waiting
            char tempBuffer[BUF_SIZE];
            tempBuffer[0] = 0;
            if (!check_socket(client->sock, tempBuffer))
            {
               client->game = NULL;
               return SOCKET_ERROR;
            }
            else if (strcmp(tempBuffer, "quit") == 0)
            {
               client->game = NULL;
               return EXIT_SUCCESS;
            }
            else if (strlen(tempBuffer) > 0)
            {
               send_chat_message_to_all_observers(client, tempBuffer, BLUE);
            }
         }
      }
   }
   return EXIT_SUCCESS;
}

static int handleFriends(Client *client)
{
   char buffer[BUF_SIZE];
   while (1)
   {
      write_client(client->sock, "\nWhat do you want to do ?\n" RED "cancel" RESET " to cancel\n\n");
      write_client(client->sock, "\t- " BLUE "add" RESET " : add a friend\n");
      write_client(client->sock, "\t- " BLUE "remove" RESET " : remove a friend\n");
      write_client(client->sock, "\t- " BLUE "list" RESET " : list your friends\n");

      buffer[0] = 0;
      if (read_client(client->sock, buffer) <= 0)
         return SOCKET_ERROR;
      if (strcmp(buffer, "cancel") == 0)
      {
         break;
      }
      else if (strcmp(buffer, "add") == 0)
      {
         if (!list_not_friends(buffer, client))
         {
            write_client(client->sock, "\nThere is no player available to add !\n");
            continue;
         }
         write_client(client->sock, "\nWho do you want to add ?\n" RED "cancel" RESET " to cancel\n");
         write_client(client->sock, buffer);

         buffer[0] = 0;
         if (read_client(client->sock, buffer) <= 0)
            return SOCKET_ERROR;
         if (strcmp(buffer, "cancel") == 0)
         {
            continue;
         }
         Client *friend = getClientByName(buffer);
         char message[BUF_SIZE];
         if (friend == NULL)
         {
            snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " not found !\n", buffer);
            write_client(client->sock, message);
         }
         else if (friend == client)
         {
            write_client(client->sock, "\nYou can't add yourself !\n");
         }
         else
         {
            boolean alreadyFriend = FALSE;
            for (int i = 0; i < MAX_FRIENDS; i++)
            {
               if (client->friends[i] == friend)
               {
                  alreadyFriend = TRUE;
                  break;
               }
            }
            if (alreadyFriend)
            {
               snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " is already your friend !\n", buffer);
               write_client(client->sock, message);
            }
            else
            {
               for (int i = 0; i < MAX_CLIENTS; i++)
               {
                  if (client->friends[i] == NULL)
                  {
                     client->friends[i] = friend;
                     snprintf(message, BUF_SIZE, "\nPlayer " GREEN "%s" RESET " added to your friends !\n", buffer);
                     write_client(client->sock, message);
                     break;
                  }
               }
            }
         }
      }
      else if (strcmp(buffer, "remove") == 0)
      {
         if (!list_friends(buffer, client))
         {
            write_client(client->sock, "\nYou don't have any friends to remove !\n");
            continue;
         }
         write_client(client->sock, "\nWho do you want to remove ?\n" RED "cancel" RESET " to cancel\n");
         write_client(client->sock, buffer);

         buffer[0] = 0;
         if (read_client(client->sock, buffer) <= 0)
            return SOCKET_ERROR;
         if (strcmp(buffer, "cancel") == 0)
         {
            continue;
         }
         Client *friend = getClientByName(buffer);
         char message[BUF_SIZE];
         if (friend == NULL)
         {
            snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " not found !\n", buffer);
            write_client(client->sock, message);
         }
         else if (friend == client)
         {
            write_client(client->sock, "\nYou can't remove yourself !\n");
         }
         else
         {
            boolean alreadyFriend = FALSE;
            for (int i = 0; i < MAX_FRIENDS; i++)
            {
               if (client->friends[i] == friend)
               {
                  alreadyFriend = TRUE;
                  client->friends[i] = NULL;
                  snprintf(message, BUF_SIZE, "\nPlayer " GREEN "%s" RESET " removed from your friends !\n", buffer);
                  write_client(client->sock, message);
                  break;
               }
            }
            if (!alreadyFriend)
            {
               snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " is not your friend !\n", buffer);
               write_client(client->sock, message);
            }
         }
      }
      else if (strcmp(buffer, "list") == 0)
      {
         list_friends(buffer, client);
         write_client(client->sock, buffer);
      }
      else
      {
         write_client(client->sock, RED "\nInvalid input !\n" RESET);
      }
   }
   return EXIT_SUCCESS;
}

static int handleBio(Client *client)
{
   char buffer[BUF_SIZE];
   while (1)
   {
      write_client(client->sock, "\nType your new bio (max 255 characters) :\n" RED "cancel" RESET " to cancel\n");
      buffer[0] = 0;
      if (read_client(client->sock, buffer) <= 0)
         return SOCKET_ERROR;
      if (strcmp(buffer, "cancel") == 0)
      {
         break;
      }
      else if (strlen(buffer) > BIO_SIZE)
      {
         write_client(client->sock, RED "\nBio too long !\n" RESET);
      }
      else
      {
         strncpy(client->bio, buffer, BIO_SIZE - 1);
         break;
      }
   }
   return EXIT_SUCCESS;
}

static int handleConsult(Client *client)
{
   char buffer[BUF_SIZE];
   while (1)
   {
      write_client(client->sock, "\nWhose bio do you want to consult ?\n" RED "cancel" RESET " to cancel\n");
      list_clients(buffer, client);
      write_client(client->sock, buffer);

      buffer[0] = 0;
      if (read_client(client->sock, buffer) <= 0)
         return SOCKET_ERROR;
      if (strcmp(buffer, "cancel") == 0)
      {
         break;
      }
      Client *consulted = getClientByName(buffer);
      char message[BUF_SIZE];
      if (consulted == NULL)
      {
         snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " not found !\n", buffer);
         write_client(client->sock, message);
      }
      else
      {
         if (strcmp(consulted->bio, "") == 0)
         {
            snprintf(message, BUF_SIZE, "\n%s doesn't have a bio !\n", consulted->name);
            write_client(client->sock, message);
            break;
         }
         else
         {
            snprintf(message, BUF_SIZE, "\n%s's bio :\n%s\n", consulted->name, consulted->bio);
            write_client(client->sock, message);
            break;
         }
      }
   }
   return EXIT_SUCCESS;
}

static void send_chat_message_to_all_observers(Client *client, const char *buffer, const char *color)
{
   char message[BUF_SIZE];
   message[0] = 0;
   strncat(message, "\n", BUF_SIZE - strlen(message) - 1);
   strncat(message, color, BUF_SIZE - strlen(message) - 1);
   strncat(message, client->name, BUF_SIZE - strlen(message) - 1);
   strncat(message, " : ", BUF_SIZE - strlen(message) - 1);
   strncat(message, buffer, BUF_SIZE - strlen(message) - 1);
   strncat(message, "\n", BUF_SIZE - strlen(message) - 1);
   strncat(message, RESET, BUF_SIZE - strlen(message) - 1);
   send_message_to_all_observers(client->game, message, NULL);
}

static boolean canObserve(Client *observed, Client *client)
{
   boolean canWatch = FALSE;
   if (observed->isPrivate || (observed->challengedBy != NULL && observed->challengedBy->isPrivate))
   {
      for (int j = 0; j < MAX_FRIENDS; j++)
      {
         if (observed->friends[j] == client || observed->challengedBy->friends[j] == client)
         {
            canWatch = TRUE;
            break;
         }
      }
   }
   else
   {
      canWatch = TRUE;
   }
   return canWatch;
}

static void send_message_to_all_observers(Game *game, const char *buffer, Client *except)
{
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL && clients[i]->game == game && clients[i] != except)
      {
         write_client(clients[i]->sock, buffer);
      }
   }
}

static void update_game_of_all_observers(Game *oldGame, Game *newGame)
{
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL && clients[i]->game == oldGame)
      {
         clients[i]->game = newGame;
      }
   }
}

static int challengeClient(Client *challenger)
{
   char buffer[BUF_SIZE];
   char list[BUF_SIZE];
   while (1)
   {
      boolean isPrivate = FALSE;
      write_client(challenger->sock, "\nDo you want to make a " RED "private" RESET " or " RED "public" RESET " challenge ?\n" RED "cancel" RESET " to cancel\n");
      while (1)
      {
         buffer[0] = 0;
         if (read_client(challenger->sock, buffer) <= 0)
            return SOCKET_ERROR;
         else if (strcmp(buffer, "cancel") == 0)
         {
            return EXIT_SUCCESS;
         }
         else if (strcmp(buffer, "private") == 0)
         {
            isPrivate = TRUE;
            break;
         }
         else if (strcmp(buffer, "public") == 0)
         {
            isPrivate = FALSE;
            break;
         }
         else
         {
            write_client(challenger->sock, RED "Invalid input !\n" RESET "Enter " RED "private" RESET " or " RED "public" RESET "\n");
         }
      }

      write_client(challenger->sock, "\nWho do you want to challenge ?\n" RED "cancel" RESET " to cancel\n");
      list_clients_not_in_game(buffer, challenger);
      write_client(challenger->sock, buffer);

      buffer[0] = 0;
      if (read_client(challenger->sock, buffer) <= 0)
         return SOCKET_ERROR;
      if (strcmp(buffer, "cancel") == 0)
      {
         break;
      }
      Client *challengee = getClientByName(buffer);
      char message[BUF_SIZE];
      if (challengee == NULL)
      {
         snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " not found !\n", buffer);
         write_client(challenger->sock, message);
      }
      else if (challengee->game != NULL)
      {
         snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " is already playing !\n", buffer);
         write_client(challenger->sock, message);
      }
      else if (challenger->challengedBy != NULL)
      {
         snprintf(message, BUF_SIZE, "\nYou are already challenged by " RED "%s" RESET " !\n", challenger->challengedBy->name);
         write_client(challenger->sock, message);
      }
      else if (challengee == challenger)
      {
         snprintf(message, BUF_SIZE, "\nYou can't challenge yourself !\n");
         write_client(challenger->sock, message);
      }
      else if (challengee->challengedBy != NULL)
      {
         snprintf(message, BUF_SIZE, "\nPlayer " RED "%s" RESET " is already challenged !\n", buffer);
         write_client(challenger->sock, message);
      }
      else
      {
         challengee->challengedBy = challenger;
         challenger->challengedBy = challengee;

         // Before creating the game, set if it's private or not
         // It can be checked only here since game is created just after this event
         challengee->isPrivate = isPrivate;
         challenger->isPrivate = isPrivate;

         snprintf(message, BUF_SIZE, "\n%s wants to challenge you in a " RED "%s" RESET " game!\n", challenger->name, isPrivate ? "private" : "public");
         write_client(challengee->sock, message);

         snprintf(message, BUF_SIZE, "\nWaiting for %s's response...\n", challengee->name);
         write_client(challenger->sock, message);
         break;
      }
   }
   // Wait for the challengee to accept or refuse
   while (challenger->challengedBy != NULL)
   {
      char tempBuffer[BUF_SIZE];
      // Check for disconnect while waiting
      if (!check_socket(challenger->sock, tempBuffer))
      {
         return SOCKET_ERROR;
      }
      // If a game is created, it means the other accepted it
      else if (challenger->game != NULL)
      {
         break;
      }
   }
   return EXIT_SUCCESS;
}

static void app(void)
{
   SOCKET sock = init_connection();

   /* init the array of threads */
   pthread_t threads[MAX_THREADS];

   char buffer[BUF_SIZE];
   int max = sock;

   fd_set rdfs;

   while (1)
   {
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if (read_client(csock, buffer) == SOCKET_ERROR)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         // Create the client
         Client *client = (Client *)malloc(sizeof(Client));
         client->sock = csock;
         client->game = NULL;
         client->player = NULL;
         client->challengedBy = NULL;
         strncpy(client->name, buffer, BUF_SIZE - 1);
         int index = add_client(client);

         // If client couldn't be added
         if (index < 0)
         {
            switch (index)
            {
            case MAX_PLAYERS_ERROR:
               write_client(csock, "\nServer is full !\n");
               break;
            case USERNAME_TAKEN_ERROR:
               write_client(csock, "\nUsername already taken !\n");
               break;
            case USERNAME_EMPTY_ERROR:
               write_client(csock, "\nUsername can't be empty !\n");
               break;
            }
            end_connection(csock);
         }
         else
         {
            // Welcome message
            char message[BUF_SIZE];
            snprintf(message, BUF_SIZE, "\nWelcome " GREEN "%s" RESET " to the Awale Server !\n", client->name);
            write_client(csock, message);
            printf("%s is connected !\n", client->name);

            // Create dedicated thread for the client
            pthread_t thread;
            int threadResult = pthread_create(&thread, NULL, clientHandler, &index);
            if (threadResult != 0)
            {
               printf("error creating thread\n");
               exit(-1);
            }
            threads[index] = thread;
         }
      }
   }

   // If server is stopped, stop all the clients
   for (int i = 0; i < MAX_THREADS; i++)
   {
      pthread_kill(threads[i], 0);
   }
   clear_all_clients();
   end_connection(sock);
}

static void clear_client(int index)
{
   // Client was in a game
   if (clients[index]->game != NULL)
   {
      char message[BUF_SIZE];
      snprintf(message, BUF_SIZE, "By forfeit," GREEN " %s won the game !\n" RESET, clients[index]->challengedBy->name);
      send_message_to_all_observers(clients[index]->game, message, NULL);
      free_player(clients[index]->game->players[0]);
      free_player(clients[index]->game->players[1]);
      free_game(clients[index]->game);
      update_game_of_all_observers(clients[index]->game, NULL);
      clients[index]->player = NULL;
      clients[index]->challengedBy->player = NULL;
   }
   // Client was challenged
   if (clients[index]->challengedBy != NULL)
   {
      clients[index]->challengedBy->challengedBy = NULL;
      if (clients[index]->challengedBy->game == NULL)
      {
         char message[BUF_SIZE];
         snprintf(message, BUF_SIZE, "\n%s disconnected !\n", clients[index]->name);
         write_client(clients[index]->challengedBy->sock, message);
      }
   }
   closesocket(clients[index]->sock);
   free(clients[index]);
   clients[index] = NULL;
}

static void clear_all_clients()
{
   int i = 0;
   for (i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL)
      {
         clear_client(i);
      }
   }
}

static void send_message_to_all_clients(Client *sender, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] != NULL && sender->sock != clients[i]->sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender->name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i]->sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
