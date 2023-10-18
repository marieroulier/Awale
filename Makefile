SERVER_EXE = server
CLIENT_EXE = client
COMPILER = gcc
FLAGS = -c -ansi -pedantic -Wall # -Werror
DEBUG_FLAGS = -g
PERF_FLAGS = -DPERF
BUILD_PATH = ./build
BIN_PATH = ./bin
OBJETS = client.o server.o
OBJETS_PATH = $(addprefix $(BUILD_PATH)/, $(OBJETS))
CLIENT_PATH = ./client
SERVER_PATH = ./server
INCLUDE_PATH = -I ./interfaces

$(SERVER_EXE): $(SERVER_PATH)/server.c # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(SERVER_EXE) $(SERVER_PATH)/server.c $(INCLUDE_PATH)

$(CLIENT_EXE): $(CLIENT_PATH)/client.c # $(OBJETS_PATH)
	$(COMPILER) -o $(BIN_PATH)/$(CLIENT_EXE) $(CLIENT_PATH)/client.c $(INCLUDE_PATH)

# $(TEST_EXE): $(CONTROLLER_PATH)/tester.cpp $(OBJETS_PATH)
# 	$(COMPILER) -o $(BUILD_PATH)/$(TEST_EXE) $(CONTROLLER_PATH)/tester.cpp $(OBJETS_PATH) $(INCLUDE_PATH)


clean:
	rm $(BIN_PATH)/$(SERVER_EXE) $(BIN_PATH)/$(CLIENT_EXE) $(OBJETS_PATH)