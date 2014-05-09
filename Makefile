CC = g++
LDFLAGS = -lpthread
CFLAGS = -O2 -g -Wall
SERVER = server
CLIENT = client
SERVER_OBJ = server.o
CLIENT_OBJ = client.o
MD5_OBJ = md5.o
ENCRYPT_OBJ = encrypt.o
SERVER_FILES = server.cpp md5.cpp encrypt.cpp
CLIENT_FILES = client.cpp md5.cpp encrypt.cpp
ALL:$(SERVER) $(CLIENT)

$(SERVER):$(SERVER_FILES)
	$(CC) -o $@ $(SERVER_FILES) $(LDFLAGS) $(CFLAGS)
$(CLIENT):$(CLIENT_FILES)
	$(CC) -o $@ $(CLIENT_FILES) $(CFLAGS) $(LDFLAGS)


.PHONY:clean
clean:
	rm -rf $(SERVER) $(SERVER_OBJ)
	rm -rf $(CLIENT) $(CLIENT_OBJ)
