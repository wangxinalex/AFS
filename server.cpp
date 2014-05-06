/*
 * =====================================================================================
 *
 *       Filename:  server.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/05/14 05:45:35
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include "server.h"
#include <algorithm>
#define PORT "32001"
#define BACKLOG 10
#define BUF_LEN 512
#define CLIENT_QUIT -2
#define MAX_CLIENT 20
using namespace std;
static int global_max_client = 0;
vector<client> client_list;
vector<file_node> file_list;
const string server_dir = "server_dir";
int max_file_uid = 0;

int main(int argc, char* argv[]){
	pthread_t thread;
	int sock;
	struct addrinfo hints, *res;
	int reuseaddr = 1;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(NULL, PORT, &hints, &res) != 0){
		perror("[ERROR] getaddrinfo");
		return 1;
	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(sock == -1){
		perror("[ERROR] socket");
		return 1;
	}
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int))==-1){
		perror("[ERROR] setsockopt");
		return 1;
	}

	if(bind(sock, res->ai_addr, res->ai_addrlen) == -1){
		perror("[ERROR] bind");
		return 1;
	}

	freeaddrinfo(res);
	
	if(listen(sock, BACKLOG) == -1){
		perror("[ERROR] listen");
		return 1;
	}

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(server_dir.c_str()))==NULL){
		perror("[ERROR] open directory");
	}else{
		while((dirp = readdir(dp)) != NULL){
			if(strcmp(dirp->d_name, ".")==0||strcmp(dirp->d_name, "..")==0){
				continue;
			}
			file_node new_file(max_file_uid++, dirp->d_name);
			file_list.push_back(new_file);
		}
	}
	dump_file_list();

	while(1){
		size_t size = sizeof(struct sockaddr_in);
		struct sockaddr_in client_addr;
		int newsock = accept(sock, (struct sockaddr*)&client_addr, &size);
		if(newsock == -1){
			perror("[ERROR] accept");
		}else{
			printf("Find a connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
			pthread_create(&thread, NULL, handle, &newsock);
		}

	}
	close(sock);
	return 0;
}

void *handle(void *p){
	int newsock  = *(int*)p;
	int new_client_id = add_client(newsock);
	dump_list();
	int recv_byte = 0;
	char recv_buf[BUF_LEN];
	int return_value;
	while((recv_byte = read(newsock, recv_buf, sizeof(recv_buf) - 1)) != 0){
		recv_buf[recv_byte] = 0;
		printf("Received buffer: %s\n", recv_buf );
		return_value = echo_command(new_client_id, recv_buf );
		if(return_value == CLIENT_QUIT ){
			break;
		}
	}
	return NULL;
}

int create_file(int client_fd, char * command){
	char file_name[BUF_LEN];
	if(sscanf(command, "create %s", file_name) == 0){
		perror("[ERROR] command format error");
		return 1;
	}
	printf("Create %d %s\n", client_fd, file_name);
	return 0;
}

int open_file(int client_fd,char * command){
	printf("Open\n");
	
	return 0;
}
int read_file(int client_fd,char * command){
	printf("Read\n");
	return 0;
}
int close_file(int client_fd,char * command){
	printf("Close\n");
	return 0;
}
int delete_file(int client_fd, char * command){
	printf("Delete\n");
	return 0;
}
int status_file(int client_fd,char * command){
	printf("Status\n");
	return 0;
}

int quit(int client_fd, char* command){
	printf("Client %d Quit\n", client_fd);
	vector<client>::iterator iter;
	int client_sock = -1;
	for (iter = client_list.begin(); iter != client_list.end(); iter++) {
		if(iter->get_client_id() == client_fd){
			client_sock = iter->get_sock_fd();
			client_list.erase(iter);
			break;
		}
	}
	if(client_sock == -1){
		printf("No such a client\n");
		return 1;
	}
	string message = "Quit";
	if(write(client_sock, message.c_str(),message.size()) < 0){
		perror("[ERROR] write error");
	}
	close(client_sock);
	return CLIENT_QUIT;
}

int echo_command(int client_id, char * command){
	int return_value = 0;
	for(int i = 0; i < COMMAND_NUM; i++){
		const char * func_name = command_list[i].name.c_str();
		if(strncasecmp(command, func_name, strlen(func_name)) == 0){
			return_value = command_list[i].func(client_id, command);
		}
	}
	return return_value;
}

void dump_list(void){
	for(vector<client>::const_iterator iter = client_list.begin(); iter != client_list.end(); iter++){
		cout<<*iter;
	}		
}		/* -----  end of function dump_list  ----- */

vector<client>::const_iterator get_client(int client_id){
	return find_if(client_list.begin(), client_list.end(), Client_equal(client_id));
}

int add_client(int sock_fd){
	int client_fd = __sync_fetch_and_add(&global_max_client, 1);
	client new_client(client_fd, sock_fd);
	client_list.push_back(new_client);
	return client_fd;
}

void dump_file_list(){
	for(vector<file_node>::const_iterator iter = file_list.begin(); iter != file_list.end(); iter++){
		cout << *iter;
	}
}
