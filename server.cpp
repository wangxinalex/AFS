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
#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>
#include "util.h"
#include "server.h"
#include "md5.h"
#define PORT "32001"
#define BACKLOG 10
using namespace std;
static int global_max_client = 0;
vector<client> client_list;
vector<file_node> file_list;
const string server_dir = "server_dir/";
int max_file_uid = 0;
char dummy[MAX_NAME];
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
	dump_client_list();
	int recv_byte = 0;
	char recv_buf[MAX_BUFF];
	int return_value;
	while((recv_byte = read(newsock, recv_buf, sizeof(recv_buf) - 1)) != 0){
		recv_buf[recv_byte] = 0;
		printf("[INST] %s\n", recv_buf );
		return_value = echo_command(new_client_id, recv_buf );
		if(return_value == CLIENT_QUIT ){
			break;
		}
	}
	return NULL;
}

int pass_client(int client_fd, const char* content){
	printf("[SEND to %d] %s\n",client_fd,content );
	vector<client>::const_iterator client_iter = get_client(client_fd);
	if(client_iter == client_list.end()){
		fprintf(stderr,"[ERROR] No such a client %d\n",client_fd);
		return FILE_ERR;
	}
	int sock_fd = client_iter->get_sock_fd();
	if(write(sock_fd, content, strlen(content)) < 0){
		fprintf(stderr,"[ERROR] write to client error: %s, sock_fd: %d\n", content,sock_fd );
		return FILE_ERR;
	}
	return 0;
}

int recv_client(int client_fd, char* buffer, int size){
	memset(buffer, 0, size);
	vector<client>::const_iterator client_iter = get_client(client_fd);
	if(client_iter == client_list.end()){
		fprintf(stderr,"[ERROR] No such a client %d\n",client_fd);
		return FILE_ERR;
	}
	if(read(client_iter->get_sock_fd(), buffer, size)<0){
		fprintf(stderr, "[ERROR] read from client %d failed\n", client_fd);
		return FILE_ERR;	
	}
	printf("[RECV from %d] %s\n", client_fd, buffer);
	return 0;
}

int pass_client_file(int client_fd,int file_fd){
	char response[MAX_RESPONSE];
	FILE* fp = fdopen(file_fd, "r");
	if(fp == NULL){
		perror("[ERROR] file open error");
		return FILE_ERR;
	}
	struct stat file_stat;
	fstat(file_fd, &file_stat);
	long int file_length = file_stat.st_size;
	vector<client>::const_iterator client_iter = get_client(client_fd);
	int sock_fd = client_iter->get_sock_fd();
	char trans_msg[MAX_RESPONSE];
	memset(trans_msg,0,sizeof(trans_msg));
	sprintf(trans_msg, "%s %ld", TRANS_FILE_START, file_length);
	if(pass_client(client_fd, trans_msg)!=0){
		fprintf(stderr,"File Transmission Error\n");
		return FILE_ERR;
	}
	printf("Waiting for ACK\n");
	if(recv_client(client_fd, response, MAX_RESPONSE)!=0||strncmp(response, TRANS_FILE_START_ACK, strlen(TRANS_FILE_START_ACK)!=0)){
		fprintf(stderr, "[ERROR] Transmission file failed\n");
		return FILE_ERR;
	}
	if(file_length>0){
		printf("File Transmission Start\n");
		char buffer[MAX_BUFF];
		memset(buffer, 0, sizeof(buffer));
		int read_byte = 0;
		long int total_byte = 0;
		while((read_byte = fread(buffer,sizeof(char), MAX_BUFF, fp))!=0){
			if(write(sock_fd, buffer, strlen(buffer))<0){
				fprintf(stderr,"Send file filed\n");
				return FILE_ERR;
			}
			total_byte+=read_byte;
			if(read_byte != MAX_BUFF){
				printf("total_read_byte = %ld\n", total_byte);
				if(ferror(fp)){
					fprintf(stderr,"File Read Error\n");
				}
				break;
			}
			memset(buffer, 0, sizeof(buffer));
		}
		printf("File Transmission Finished\n");
	}
	fflush(fp);
	fclose(fp);
	return 0;
}

int create_file(int client_fd, char * command){
	char file_name[MAX_BUFF];
	if(sscanf(command, "create %s", file_name) == 0){
		perror("[ERROR] command format error");
		return 1;
	}
	printf("Create %d %s\n", client_fd, file_name);
	return 0;
}

int open_file(int client_fd,char * command){
	vector<client>::const_iterator client_iter = get_client(client_fd);
	char file_name[MAX_NAME];
	memset(file_name,0,sizeof(MAX_NAME));
	if(sscanf(command,"open %s",file_name)<=0||strlen(file_name)==0){
		perror("[ERROR] format error");
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = get_file(file_name);
	string file_path = server_dir + file_name;
	int file_fd = -1;
	if(iter == file_list.end()){
		//not exist, create the file
		printf("Create file %s\n", file_name);
		file_fd = open(file_path.c_str(), O_RDWR|O_APPEND|O_CREAT, RWRWRW);
		if(file_fd < 0){
			perror("[ERROR] file create error");
			return FILE_ERR;
		}
		file_node new_file(max_file_uid++, file_name, file_fd);
		new_file.promise_list.push_back(client_fd);
		file_list.push_back(new_file);
	}else if(iter->get_file_lock() == exclusive_lock){
		//exclusive lock
		if(pass_client(client_fd, LOCK_MES)!=0){
			fprintf(stderr, "write to client error\n");
			return FILE_ERR;
		}
		return LOCK_ERR;
	}else{
		//exist, fetch the file
		printf("Fetch file %s\n", file_name);
		file_fd = iter->get_file_des();	
		if(file_fd<0){
			file_fd = open(file_path.c_str(), O_RDWR|O_APPEND|O_CREAT, RWRWRW);
			if(file_fd < 0){
				perror("[ERROR] file create error");
				return FILE_ERR;
			}
			iter->set_file_des(file_fd);
		}
	}
	if(pass_client_file(client_fd, file_fd)!=0){
		fprintf(stderr,"[ERROR] file %s transmission error\n" , file_name);
		return FILE_ERR;
	}
	printf("Open file finished\n");
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

int setlock_file(int client_fd, char * command){
	printf("Setlock\n");
	char lock_type[MAX_RESPONSE];
	char file_name[MAX_NAME];
	memset(file_name,0,sizeof(file_name));
	memset(lock_type,0,sizeof(lock_type));
	if(sscanf(command,"%s %s %s",dummy, file_name, lock_type)!=3){
		fprintf(stderr, "[ERROR] Format error\n");
		pass_client(client_fd,LOCK_FAIL);
		return FORMAT_ERR;
	}
	lock_t lock;
	if(strncasecmp(lock_type, SHARED_LOCK, strlen(SHARED_LOCK))==0){
		lock = shared_lock;
	}else if(strncasecmp(lock_type, EXCLUSIVE_LOCK, strlen(EXCLUSIVE_LOCK))==0){
		lock = exclusive_lock;
	}else{
		fprintf(stderr,"[ERROR] lock type error\n");
		pass_client(client_fd,LOCK_FAIL);
		return LOCK_ERR;
	}
	vector<file_node>::iterator iter = get_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr, "No such file\n");
		pass_client(client_fd,LOCK_FAIL);
		return FILE_ERR;
	}
	if(iter->get_file_lock()!=no_lock){
		pass_client(client_fd,LOCK_FAIL);
		printf("Already lccked\n");
	}else if(lock == exclusive_lock && !iter->promise_list.empty()){
		pass_client(client_fd,LOCK_FAIL);
		printf("Others have copy\n");
	}else{
		iter->set_file_lock(lock);
		pass_client(client_fd,LOCK_SUCCESS);
		printf("Lock Success\n");
	}
	dump_file_list();
	return 0;
}
int unsetlock_file(int client_fd, char * command){
	printf("Unsetlock\n");
	char file_name[MAX_NAME];
	memset(file_name,0,sizeof(file_name));
	if(sscanf(command,"%s %s", dummy,file_name)!=2){
		fprintf(stderr, "Format error\n");
		pass_client(client_fd, GENERAL_FAIL);
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = get_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr,"[ERROR] No such file\n");
		pass_client(client_fd, GENERAL_FAIL);
		return FILE_ERR;
	}
	iter->set_file_lock(no_lock);
	pass_client(client_fd, GENERAL_SUCCESS);
	return 0;
}

int removecallback_file(int client_fd,char * command){
	printf("RemoveCallback\n");
	char file_name[MAX_NAME];
	memset(file_name,0,sizeof(file_name));
	if(sscanf(command,"%s %s", dummy,file_name)!=2){
		fprintf(stderr, "Format error\n");
		pass_client(client_fd, GENERAL_FAIL);
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = get_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr,"[ERROR] No such file\n");
		pass_client(client_fd, GENERAL_FAIL);
		return FILE_ERR;
	}
	vector<int>::iterator piter;
	for(piter = iter->promise_list.begin();piter!=iter->promise_list.end();piter++){
		if(*piter == client_fd){
			break;
		}
	}
	if(piter == iter->promise_list.end()){
		fprintf(stderr, "No promise for %d\n",client_fd);
		pass_client(client_fd, GENERAL_FAIL);
		return FILE_ERR;
	}
	iter->promise_list.erase(piter);
	pass_client(client_fd, GENERAL_SUCCESS);
	printf("RemoveCallback finished\n");
	return 0;
}
int addcallback_file(int client_fd,char * command){
	printf("AddCallback\n");
	char file_name[MAX_NAME];
	memset(file_name,0,sizeof(file_name));
	if(sscanf(command,"%s %s", dummy,file_name)!=2){
		fprintf(stderr, "Format error\n");
		pass_client(client_fd, GENERAL_FAIL);
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = get_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr,"[ERROR] No such file\n");
		pass_client(client_fd, GENERAL_FAIL);
		return FILE_ERR;
	}
	vector<int>::iterator piter;
	for(piter = iter->promise_list.begin();piter!=iter->promise_list.end();piter++){
		if(*piter == client_fd){
			break;
		}
	}
	if(piter != iter->promise_list.end()){
		fprintf(stderr, "Already promise for %d\n",client_fd);
		pass_client(client_fd, GENERAL_FAIL);
		return FILE_ERR;
	}
	iter->promise_list.push_back(client_fd);
	pass_client(client_fd, GENERAL_SUCCESS);
	printf("AddCallback finished\n");
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

vector<file_node>::iterator get_file(int uid){
	return find_if(file_list.begin(), file_list.end(), File_equ(uid));	
}

vector<file_node>::iterator get_file(char * name){
	return find_if(file_list.begin(), file_list.end(), File_equ_str(name));	
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

void dump_client_list(void){
	for(vector<client>::const_iterator iter = client_list.begin(); iter != client_list.end(); iter++){
		cout<<*iter;
	}		
}		/* -----  end of function dump_client_list  ----- */

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
