#ifndef CLIENT_H
#define CLIENT_H

#define BUF_SIZE 1024

typedef int SOCKET;
typedef int boolean;

#include "gameLogic.h"
typedef struct _Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   Game *game;
   Player *player;
   struct _Client *challengedBy;
} Client;

#endif /* guard */
