CC = g++
LDFLAGS = -lpthread
CFLAGS = -O2 -g -Wall
SERVER = server
CLIENT = client
SERVER_OBJ = server.o
CLIENT_OBJ = client.o
ALL:$(SERVER) $(CLIENT)

$(SERVER):$(SERVER_OBJ)	
	$(CC) -o $(SERVER) $(SERVER_OBJ) $(CFLAGS) $(LDFLAGS)
$(CLIENT):$(CLIENT_OBJ)
	$(CC) -o $(CLIENT) $(CLIENT_OBJ) $(CFLAGS) $(LDFLAGS)

$(SERVER_OBJ) $(CLIENT_OBJ):%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -rf $(SERVER) $(SERVER_OBJ)
	rm -rf $(CLIENT) $(CLIENT_OBJ)
