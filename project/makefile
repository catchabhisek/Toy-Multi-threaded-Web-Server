SRC_DIR = ./src/
OBJ_DIR = ./obj/
LIB_DIR = ./lib/
DATA_DIR = ./data/
BIN_DIR = ./bin/
SCRIPT_DIR = ./scripts/

CC = gcc
LDLIBS = -lmyOwnthread
CFLAGS = -Wall -fpic
TARGET = libmyOwnthread.so
CFLAGS-PROG = -L$(LIB_DIR)

MAX_CONNEC = 3


$(TARGET): $(LIB_DIR)userthread.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $(OBJ_DIR)userthread.o -c $(LIB_DIR)userthread.c
	$(CC) -o $(LIB_DIR)libmyOwnthread.so $(OBJ_DIR)userthread.o -shared

LD_LIBRARY_PATH=${CURDIR}/lib/

.PHONY: clean all matrix boundedBuffer

server: $(SRC_DIR)server.c $(LIB_DIR)libmyOwnthread.so
	export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)
	$(CC) -o $(BIN_DIR)server $(SRC_DIR)server.c $(CFLAGS-PROG) $(LDLIBS)
	$(BIN_DIR)server $(MAX_CONNEC)

plot: $(DATA_DIR)time_size.gnuplot $(DATA_DIR)size_time_http1.dat $(DATA_DIR)size_time_http1_1.dat
	gnuplot -p -c $(DATA_DIR)time_size.gnuplot

client1: $(SCRIPT_DIR)client_1.c $(BIN_DIR)server
	$(CC) -o $(BIN_DIR)client1 $(SCRIPT_DIR)client_1.c $(CFLAGS-PROG) $(LDLIBS)
	$(BIN_DIR)client1

client1_1: $(SCRIPT_DIR)client_1_1.c $(BIN_DIR)server
	$(CC) -o $(BIN_DIR)client1_1 $(SCRIPT_DIR)client_1_1.c $(CFLAGS-PROG) $(LDLIBS)
	$(BIN_DIR)client1_1

connoverflow: $(SCRIPT_DIR)overflow.c $(BIN_DIR)server
	$(CC) -o $(BIN_DIR)overflow $(SCRIPT_DIR)overflow.c $(CFLAGS-PROG) $(LDLIBS)
	$(BIN_DIR)overflow

clean:
	rm -rf $(OBJ_DIR)*.o $(BIN_DIR)* $(LIB_DIR)*.so
