#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
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
      int c = read_client(client->sock, buffer);
      if (c <= 0)
      {
         break;
      }
      else
      {
         printf("Received message from %s: %s\n", client->name, buffer);
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
         client->isChallenged = FALSE;
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

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
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

// implémenter dans app l'attente du message du client pour challenger qqun d'autre / être challengé
