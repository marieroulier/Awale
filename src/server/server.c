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
   char buffer[BUF_SIZE];

   fd_set rdfs;
   FD_ZERO(&rdfs);
   FD_SET(client->sock, &rdfs);

   while (1)
   {
      while (client->game != NULL)
      {
         // TODO : handle disconnections during game
         Game *game = client->game;
         char buffer[BUF_SIZE];
         boolean quit = FALSE;
         while (game != NULL && game->turn != client->player)
         {
            if (!check_socket(client->sock))
            {
               quit = TRUE;
               break;
            }
         }
         if (game == NULL || quit)
         {
            break;
         }
         write_client(client->sock, "\n" RED "It's your turn !\n" RESET);
         construct_board(game, buffer);
         write_client(client->sock, buffer);
         if (read_client(client->sock, buffer) <= 0)
         {
            break;
         }
         else
         {
            int caseNumber = atoi(buffer);
            if (caseNumber == 0)
            {
               write_client(client->sock, RED "Invalid move\nPlease retry another one.\n" RESET);
               continue;
            }
            Pit pit;
            if (!get_pit(caseNumber, &pit))
            {
               tie(game);
               game->turn = get_opponent(game->turn, game);
               continue;
            }
            else if (is_valid_move(pit, game))
            {
               make_move(&game, pit);
               write_client(client->sock, "\nMove done !\nWait for your turn...\n");
            }
            else
            {
               write_client(client->sock, RED "Invalid move\nPlease retry another one.\n" RESET);
            }

            // TODO : handle game over
         }
      }
      if (FD_ISSET(client->sock, &rdfs))
      {
         list_commands(client);
         buffer[0] = 0;
         int c = read_client(client->sock, buffer);
         if (c <= 0)
         {
            break;
         }
         else
         {
            if (strcmp(buffer, "list") == 0)
            {
               list_clients(buffer, client);
               write_client(client->sock, buffer);
            }
            else if (strcmp(buffer, "challenge") == 0)
            {
               if (challengeClient(client) == SOCKET_ERROR)
                  break;
               ;
            }
            else if (strcmp(buffer, "accept") == 0)
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
            else if (strcmp(buffer, "quit") == 0)
            {
               break;
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
      }
   }

   printf("Client %s disconnected\n", client->name);
   clear_client(index);
   return NULL;
}

static boolean check_socket(int sockFd)
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
      char temp_buffer[BUF_SIZE];
      int bytes_read = read_client(sockFd, temp_buffer);
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
   strncat(buffer, "\t- " BLUE "challenge" RESET " : challenge a player\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "accept" RESET " : accept a challenge\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "refuse" RESET " : refuse a challenge\n", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\t- " BLUE "quit" RESET " : quit the game\n", BUF_SIZE - strlen(buffer) - 1);
   write_client(client->sock, buffer);
}

static void list_clients(char *buffer, Client *client)
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

static int challengeClient(Client *challenger)
{
   char buffer[BUF_SIZE];
   while (1)
   {
      write_client(challenger->sock, "\nWho do you want to challenge ?\n" RED "cancel" RESET " to cancel\n");
      list_clients(buffer, challenger);
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

         snprintf(message, BUF_SIZE, "\n%s wants to challenge you!\n", challenger->name);
         write_client(challengee->sock, message);

         snprintf(message, BUF_SIZE, "\nWaiting for %s's response...\n", challengee->name);
         write_client(challenger->sock, message);
         break;
      }
   }
   while (challenger->challengedBy != NULL)
   {
      if (!check_socket(challenger->sock))
      {
         return SOCKET_ERROR;
      }
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

         Client *client = (Client *)malloc(sizeof(Client));
         client->sock = csock;
         client->game = NULL;
         client->player = NULL;
         client->challengedBy = NULL;
         strncpy(client->name, buffer, BUF_SIZE - 1);
         int index = add_client(client);

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
   if (clients[index]->game != NULL)
   {
      char message[BUF_SIZE];
      snprintf(message, BUF_SIZE, "By forfeit," GREEN " %s won the game !\n" RESET, clients[index]->challengedBy->name);
      write_client(clients[index]->challengedBy->sock, message);
      free_player(clients[index]->game->players[0]);
      free_player(clients[index]->game->players[1]);
      free(clients[index]->game);
      clients[index]->challengedBy->game = NULL;
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
