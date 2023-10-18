SERVER_EXE = server
CLIENT_EXE = client
GAME_EXE = game
COMPILER = gcc
FLAGS = -c -ansi -pedantic -Wall # -Werror
DEBUG_FLAGS = -g
PERF_FLAGS = -DPERF
BUILD_PATH = ./build
BIN_PATH = ./bin
OBJETS = client.o server.o gameLogic.o
OBJETS_PATH = $(addprefix $(BUILD_PATH)/, $(OBJETS))
SRC_PATH = ./src
CLIENT_PATH = $(SRC_PATH)/client
SERVER_PATH = $(SRC_PATH)/server
INCLUDE_PATH = -I ./interfaces

$(SERVER_EXE): $(SERVER_PATH)/server.c # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(SERVER_EXE) $(SERVER_PATH)/server.c $(INCLUDE_PATH)

$(CLIENT_EXE): $(CLIENT_PATH)/client.c # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(CLIENT_EXE) $(CLIENT_PATH)/client.c $(INCLUDE_PATH)

$(GAME_EXE): $(SERVER_PATH)/gameLogic.c # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(GAME_EXE) $(SERVER_PATH)/gameLogic.c $(INCLUDE_PATH)


clean:
	rm $(BIN_PATH)/$(SERVER_EXE) $(BIN_PATH)/$(CLIENT_EXE) $(BIN_PATH)/$(GAME_EXE) $(OBJETS_PATH)