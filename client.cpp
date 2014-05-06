/*
 * =====================================================================================
 *
 *       Filename:  client.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/05/14 07:52:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include "client.h"
#define MAX_BUFF 1024
#define PORT 32001
#define SERVER "127.0.0.1"
#define MAX_FILE 200
#define RWRWRW (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
using namespace std;
vector<file_node> file_list;
const string client_dir = "client_dir/";
int max_file_uid = 0;
char dummy[MAX_NAME];

int main(int argc, char* argv[]){
	int sockfd = 0;
	char recvBuff[MAX_BUFF];
	struct sockaddr_in serv_addr;
	memset(recvBuff, 0, sizeof(recvBuff));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("[ERROR] cannot create socket");
		return 1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	if(inet_pton(AF_INET, SERVER, &serv_addr.sin_addr)<=0){
		perror("[ERROR] inet_pton");
		return 1;
	}

	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){
		perror("[ERROR] connect failed");
		return 1;
	}
	char input[MAX_BUFF];

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(client_dir.c_str()))==NULL){
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
	while(fgets(input, MAX_BUFF, stdin) != NULL){
		input[strlen(input)-1] = '\0';
		echo_command(sockfd, input);
	}
	return 0;
}

int echo_command(int sockfd , char * command){
	int return_value = 0;
	int i = 0;
	for(i = 0; i < COMMAND_NUM; i++){
		const char * func_name = command_list[i].name.c_str();
		if(strncasecmp(command, func_name, strlen(func_name)) == 0){
			return_value = command_list[i].func(sockfd, command);
			break;
		}
	}
	if(i == COMMAND_NUM){
		printf("Unknown command\n");
	}
	return return_value;
}

vector<file_node>::iterator get_file(int uid){
	return find_if(file_list.begin(), file_list.end(), File_equ(uid));	
}

vector<file_node>::iterator get_file(char * name){
	return find_if(file_list.begin(), file_list.end(), File_equ_str(name));	
}
void dump_file_list(){
	for(vector<file_node>::const_iterator iter = file_list.begin(); iter != file_list.end(); iter++){
		cout << *iter;
	}
}

int open_file(int sockfd, char* command){
	char file_name[MAX_NAME];
	memset(file_name, 0, sizeof(file_name));
	if(sscanf(command, "%s %s", dummy, file_name) == 0||strlen(file_name)==0){
		printf("Format error\n");
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = get_file(file_name);
	if(iter == file_list.end()){
		printf("No such a local file\n");
		recv_file(sockfd, command, file_name);
	}else if(iter->get_file_des() == 0){
		printf("No callback promise\n");
		recv_file(sockfd, command, file_name);
	}else{
		printf("Cached files\n");	
		string file_path = client_dir + file_name;
		int file_fd = open(file_path.c_str(), O_RDWR|O_APPEND|O_CREAT, RWRWRW);	
		if(file_fd == -1){
			fprintf(stderr,"file %s open error\n", file_name);
			return FILE_ERR;
		}
		iter->set_file_des(file_fd);
	}
	return 0;
}

int recv_file(int sockfd, char* command, char* file_name){
	pass_server(sockfd, command);
	string file_path = client_dir + file_name;
	cout << file_path<<endl;
	FILE* fp = fopen(file_path.c_str(), "w+");	
	if(fp == NULL){
		fprintf(stderr,"[ERROR] file %s open error\n", file_name);
		return FILE_ERR;
	}
	char buffer[MAX_BUFF];
	memset(buffer, 0, sizeof(buffer));
	int length = 0;
	while((length = read(sockfd, buffer, MAX_BUFF)) != 0){
		if(length<0){
			fprintf(stderr,"[ERROR] recv file %s error\n",file_name);
			break;
		}
		int write_length = fwrite(buffer, sizeof(char), length, fp);
		if(write_length<length){
			fprintf(stderr,"[ERROR] file %s write error\n", file_name);
			break;
		}
		memset(buffer, 0, sizeof(buffer));
	}
	fclose(fp);
	return 0;
}

int read_file(int sockfd, char* command){
	printf("Read\n");
	dump_file_list();
	char file_name[MAX_NAME];
	memset(file_name, 0, sizeof(file_name));
	if(sscanf(command,"read %s", file_name)==0||strlen(file_name)==0){
		fprintf(stderr,"[ERROR] format error\n");
		return FORMAT_ERR;
	}
	//string file_path = client_dir + file_name;
	vector<file_node>::const_iterator iter = get_file(file_name);
	if(iter == file_list.end()||iter->get_file_des() == -1){
		fprintf(stderr,"File %s not cached or opened\n",file_name);
	}
	char buffer[MAX_BUFF];
	memset(buffer, 0, sizeof(buffer));
	FILE* fp = fdopen(iter->get_file_des(),"r");
	if(fp == NULL){
		fprintf(stderr,"[ERROR] file %s open error\n", file_name);
		return FILE_ERR;
	}
	while(fgets(buffer, MAX_BUFF, fp)!=NULL){
		if(fputs(buffer, stdout)==EOF){
			fprintf(stderr,"[ERROR] output error\n");
		}
		memset(buffer, 0, sizeof(buffer));
	}
	return 0;
}
int write_file(int sockfd, char* command){
	printf("Write\n");
	return 0;
}
int close_file(int sockfd, char* command){
	printf("Close\n");
	return 0;
}
int delete_file(int sockfd, char* command){
	printf("Delete\n");
	return 0;
}
int status_file(int sockfd, char* command){
	printf("Status\n");
	return 0;
}
int pass_server(int sockfd, char * command){
	if(write(sockfd, command, strlen(command))==-1){
		perror("[ERROR] write error");
		return SOCKET_ERR;
	}
	return 0;
}

int quit(int sockfd, char* command){
	char recv_buf[MAX_BUFF];
	int recv_byte = read(sockfd, recv_buf, sizeof(recv_buf ) -1);
	recv_buf[recv_byte] = '\0';
	if(strncmp(recv_buf, "Quit", 4) == 0){
		printf("Exited\n");
	}
	exit(0);
}
