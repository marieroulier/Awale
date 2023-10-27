#ifndef CLIENT_H
#define CLIENT_H

#define BUF_SIZE 1024
#define BIO_SIZE 256

#define MAX_CLIENTS 100
#define MAX_FRIENDS MAX_CLIENTS - 1

typedef int SOCKET;
typedef int boolean;

#include "gameLogic.h"
typedef struct _Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char bio[BIO_SIZE];
   Game *game;
   Player *player;
   boolean isPrivate;
   struct _Client *friends[MAX_FRIENDS];
   struct _Client *challengedBy;
} Client;

#endif /* guard */
