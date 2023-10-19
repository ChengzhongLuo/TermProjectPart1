# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -std=c++17


# Executables
MAIN_SERVER = servermain
CLIENT = client

all: $(MAIN_SERVER) $(CLIENT)

$(MAIN_SERVER): servermain.cpp
	$(CC) $(CFLAGS) -o $(MAIN_SERVER) servermain.cpp

$(CLIENT): client.cpp
	$(CC) $(CFLAGS) -o $(CLIENT) client.cpp

.PHONY: run-main-server run-client1 run-client2 run-client3 clean

run-main-server:
	./$(MAIN_SERVER)

run-client1:
	./$(CLIENT)

run-client2:
	./$(CLIENT)

run-client3:
	./$(CLIENT)

clean:
	rm -f $(MAIN_SERVER) $(CLIENT)
