SERVER_EXE = server
CLIENT_EXE = client
GAME_EXE = game
COMPILER = gcc
FLAGS = -c -ansi -pedantic -Wall # -Werror
DEBUG_FLAGS = -g
PERF_FLAGS = -DPERF
BUILD_PATH = ./build
BIN_PATH = ./bin
OBJETS = client.o server.o gameLogic.o display.o
OBJETS_PATH = $(addprefix $(BUILD_PATH)/, $(OBJETS))
SRC_PATH = ./src
CLIENT_PATH = $(SRC_PATH)/client
SERVER_PATH = $(SRC_PATH)/server
INCLUDE_PATH = -I ./interfaces

$(SERVER_EXE): $(SERVER_PATH)/server.c # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(SERVER_EXE) $(SERVER_PATH)/server.c $(INCLUDE_PATH) $(DEBUG_FLAGS)

$(CLIENT_EXE): $(CLIENT_PATH)/client.c # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(CLIENT_EXE) $(CLIENT_PATH)/client.c $(INCLUDE_PATH) $(DEBUG_FLAGS)

$(GAME_EXE): $(SERVER_PATH)/gameLogic.c display.o # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(GAME_EXE) $(SERVER_PATH)/gameLogic.c $(BUILD_PATH)/display.o $(INCLUDE_PATH) $(DEBUG_FLAGS)

display.o: $(SERVER_PATH)/display.c
	$(COMPILER) -c -o $(BUILD_PATH)/display.o $(SERVER_PATH)/display.c $(INCLUDE_PATH) $(DEBUG_FLAGS)


clean:
	rm -rf $(BIN_PATH)/* $(BUILD_PATH)/*