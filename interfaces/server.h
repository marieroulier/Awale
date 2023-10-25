#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#endif

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100
#define MAX_THREADS 100

#define BUF_SIZE 1024

#define MAX_PLAYERS_ERROR -1
#define USERNAME_TAKEN_ERROR -2
#define USERNAME_EMPTY_ERROR -3

#include "clientServer.h"

// Initializes the server.
static void init(void);

// Ends the server.
static void end(void);

// Runs the server.
static void app(void);

// Initializes the connection.
static int init_connection(void);

// Ends the connection.
static void end_connection(int sock);

// Reads a message from the client and puts it into buffer.
static int read_client(SOCKET sock, char *buffer);

// Writes a message to the client.
static void write_client(SOCKET sock, const char *buffer);

// Reads a message from the server and puts it into buffer.
static void send_message_to_all_clients(Client *client, const char *buffer, char from_server);

// Adds a client to the clients array.
static int add_client(Client *client);

// Lists the avaiblable commands
static void list_commands(Client *client);

// Fills the buffer with the list of clients that are available for games.
static void list_clients(char *buffer, Client *client);

// Fills the buffer with the list of clients that are playing
static void list_games(char *buffer);

// Returns a pointer to the client with the given name.
static Client *getClientByName(const char *name);

// Checks if there is a game for the client, and if so handles it until finished.
static void handleGame(Client *client);

// Handles the client's menu.
static int handleMenu(Client *client);

// Handles the observer mode.
static int handleObserver(Client *client);

// Sends to all observers the message.
static void send_message_to_all_observers(Game *game, const char *buffer);

// Updates the game of all observers.
static void update_game_of_all_observers(Game *oldGame, Game *newGame);

// The client goes into challenger mode.
static int challengeClient(Client *challenger);

// Client threaded handler.
void *clientHandler(void *indexInClients);

// Check if the socket is still alive, returns true if it is, false otherwise.
static boolean check_socket(int sockFd, char *tempBuffer);

// Frees the memory allocated for one client at index i, and closes the connections.
static void clear_client(int index);

// Frees the memory allocated for all the clients, and closes the connections.
static void clear_all_clients();

#endif /* guard */
