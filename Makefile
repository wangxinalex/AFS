CC = g++
LDFLAGS = -lpthread
CFLAGS = -O2 -g -Wall
SERVER = server
CLIENT = client
ALL:
	$(CC) -o $(SERVER) server.cpp $(CFLAGS) $(LDFLAGS)
	$(CC) -o $(CLIENT) client.cpp $(CFLAGS) $(LDFLAGS)

.PHONY:clean
clean:
	rm -rf $(SERVER)
	rm -rf $(CLIENT)
