SERVER_EXE = server
CLIENT_EXE = client
GAME_EXE = game
COMPILER = gcc
FLAGS = -c -ansi -pedantic -Wall # -Werror
DEBUG_FLAGS = -g
PERF_FLAGS = -DPERF
BUILD_PATH = ./build
BIN_PATH = ./bin
SRC_PATH = ./src
CLIENT_PATH = $(SRC_PATH)/client
SERVER_PATH = $(SRC_PATH)/server
GAME_PATH = $(SRC_PATH)/game
INCLUDE_PATH = -I ./interfaces

$(SERVER_EXE): $(SERVER_PATH)/server.c
	$(COMPILER) -o $(BIN_PATH)/$(SERVER_EXE) $(SERVER_PATH)/server.c $(INCLUDE_PATH) $(DEBUG_FLAGS)

$(CLIENT_EXE): $(CLIENT_PATH)/client.c
	$(COMPILER) -o $(BIN_PATH)/$(CLIENT_EXE) $(CLIENT_PATH)/client.c $(INCLUDE_PATH) $(DEBUG_FLAGS)

$(GAME_EXE): $(GAME_PATH)/game.c gameLogic.o display.o
	$(COMPILER) -o $(BIN_PATH)/$(GAME_EXE) $(GAME_PATH)/game.c $(BUILD_PATH)/display.o $(BUILD_PATH)/gameLogic.o $(INCLUDE_PATH) $(DEBUG_FLAGS)

gameLogic.o: $(GAME_PATH)/gameLogic.c
	$(COMPILER) -c -o $(BUILD_PATH)/gameLogic.o $(GAME_PATH)/gameLogic.c $(INCLUDE_PATH) $(DEBUG_FLAGS)

display.o: $(GAME_PATH)/display.c
	$(COMPILER) -c -o $(BUILD_PATH)/display.o $(GAME_PATH)/display.c $(INCLUDE_PATH) $(DEBUG_FLAGS)


clean:
	rm -rf $(BIN_PATH)/* $(BUILD_PATH)/*