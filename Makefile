CC = g++
ALL:
	$(CC) -o server server.cpp -lpthread
	$(CC) -o client client.cpp

.PHONY:clean
clean:
	rm -rf server
	rm -rf client
