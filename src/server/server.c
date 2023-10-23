#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "display.h"
#include "clientServer.h"
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
pthread_t threads[MAX_CLIENTS];
Game *games[MAX_GAMES];

void *clientHandler(void *indexInClients)
{
   int index = *(int *)indexInClients;
   Client *client = clients[index];
   char buffer[BUF_SIZE];
   while (1)
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
            challengeClient(client);
         }
         else if (strcmp(buffer, "accept") == 0)
         {
            // TODO : accept challenge
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

   printf("Client %s disconnected\n", client->name);
   clear_client(index);
   return NULL;
}

static int add_client(Client **clientPtr)
{
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (clients[i] == NULL)
      {
         clients[i] = *clientPtr;
         return i;
      }
   }
   return -1;
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
      if (clients[i] != NULL && clients[i]->isPlaying == FALSE && clients[i]->challengedBy == NULL)
      {
         strncat(buffer, "\t- ", BUF_SIZE - strlen(buffer) - 1);
         if (clients[i] == client)
         {
            strncat(buffer, RED, BUF_SIZE - strlen(buffer) - 1);
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

static void challengeClient(Client *challenger)
{
   char buffer[BUF_SIZE];
   while (1)
   {
      write_client(challenger->sock, "\nWho do you want to challenge ?\n" RED "cancel" RESET " to cancel\n");
      list_clients(buffer, challenger);
      write_client(challenger->sock, buffer);

      buffer[0] = 0;
      read_client(challenger->sock, buffer);
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
      else if (challengee->isPlaying == TRUE)
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
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         // TODO : check if name is already taken, if so, refuse connection
         if (read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client *client = (Client *)malloc(sizeof(Client));
         client->sock = csock;
         client->isPlaying = FALSE;
         client->challengedBy = NULL;
         strncpy(client->name, buffer, BUF_SIZE - 1);
         int index = add_client(&client);

         pthread_t thread;
         int threadResult = pthread_create(&thread, NULL, clientHandler, &index);
         if (threadResult != 0)
         {
            printf("error creating thread\n");
            exit(0);
         }
         threads[index] = thread;
      }
   }

   for (int i = 0; i < MAX_THREADS; i++)
   {
      pthread_kill(threads[i], 0);
   }
   clear_all_clients();
   end_connection(sock);
}

static void clear_client(int index)
{
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
