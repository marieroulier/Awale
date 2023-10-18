#ifndef CLIENT_H
#define CLIENT_H

#define BUF_SIZE 1024

typedef int SOCKET;
typedef int boolean;

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   boolean isPlaying;
} Client;

#endif /* guard */
