CC = g++
LDFLAGS = -lpthread
CFLAGS = -O2 -g -Wall
SERVER = server
CLIENT = client
SERVER_OBJ = server.o
CLIENT_OBJ = client.o
MD5_OBJ = md5.o
ENCRYPT_OBJ = encrypt.o
ALL:$(SERVER) $(CLIENT)

$(SERVER):server.cpp md5.cpp encrypt.cpp
	$(CC) -o $@ server.cpp md5.cpp encrypt.cpp $(LDFLAGS) $(CFLAGS)
$(CLIENT):client.cpp md5.cpp encrypt.cpp
	$(CC) -o $@ client.cpp md5.cpp encrypt.cpp $(CFLAGS) $(LDFLAGS)


.PHONY:clean
clean:
	rm -rf $(SERVER) $(SERVER_OBJ)
	rm -rf $(CLIENT) $(CLIENT_OBJ)
