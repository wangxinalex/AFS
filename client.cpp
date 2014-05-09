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
 *       Compiler:  g++
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
#include <sys/stat.h>
#include "util.h"
#include "client.h"
#include "md5.h"
#include "encrypt.h"

using namespace std;
vector<file_node> file_list;
const string server_addr = "127.0.0.1";
const int port = 32001;
string client_dir;
int max_file_uid = 0;
char dummy[MAX_NAME];

int main(int argc, char* argv[]){
	if(argc != 2){
		fprintf(stderr, "Usage:/client <home>\n");
		exit(0);
	}
	
	if(opendir(argv[1])==NULL){
		mkdir(argv[1],777);
	}
	client_dir = strcat(argv[1],"/");
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
	serv_addr.sin_port = htons(port);
	if(inet_pton(AF_INET, server_addr.c_str(), &serv_addr.sin_addr)<=0){
		perror("[ERROR] inet_pton");
		return 1;
	}

	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){
		perror("[ERROR] connect failed");
		return 1;
	}
	char input[MAX_BUFF];
	memset(input,0,MAX_BUFF);	

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(client_dir.c_str()))==NULL){
		perror("[ERROR] open directory");
	}else{
		while((dirp = readdir(dp)) != NULL){
			if(strcmp(dirp->d_name, ".")==0||strcmp(dirp->d_name, "..")==0){
				continue;
			}
			int this_file_uid = __sync_fetch_and_add(&max_file_uid,1);
			file_node new_file(this_file_uid, dirp->d_name);
			file_list.push_back(new_file);
		}
	}
	//dump_file_list();
	printf("[Input]:");
	while(fgets(input, MAX_BUFF, stdin) != NULL){
		input[strlen(input)-1] = '\0';
		echo_command(sockfd, input);
		memset(input, 0, MAX_BUFF);
		printf("[Input]:");
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
	//dump_file_list();
	return return_value;
}

vector<file_node>::iterator find_file(int uid){
	return find_if(file_list.begin(), file_list.end(), File_equ(uid));	
}

vector<file_node>::iterator find_file(char * name){
	return find_if(file_list.begin(), file_list.end(), File_equ_str(name));	
}

void dump_file_list(){
	for(vector<file_node>::const_iterator iter = file_list.begin(); iter != file_list.end(); iter++){
		cout << *iter;
	}
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  open_file
 *  Description:  open a file. Create it if not present, then synchronize it with server. If the client is in callback promise list or out-of-date, fetch the file from the server.
 * =====================================================================================
 */
int open_file(int sockfd, char* command){
	char file_name[MAX_NAME];
	memset(file_name, 0, sizeof(file_name));
	if(sscanf(command, "%s %s", dummy, file_name) == 0||strlen(file_name)==0){
		printf("Format error\n");
		return FORMAT_ERR;
	}
	int cached = 0;
	vector<file_node>::iterator iter = find_file(file_name);
	if(iter == file_list.end()){
		printf("No such a local file\n");
		int new_file_uid = __sync_fetch_and_add(&max_file_uid,1);
		file_node new_file(new_file_uid, file_name,-1,1);
		file_list.push_back(new_file);
		iter = find_file(new_file_uid);
	}else{
		cached = 1;
	}
	if(iter->get_file_des() == -1){
		printf("File %s not open\n", file_name);
		string file_path = client_dir + file_name;
		int file_fd = open(file_path.c_str(), O_RDWR|O_APPEND|O_CREAT, RWRWRW);	
		if(file_fd == -1){
			fprintf(stderr,"file %s open error\n", file_name);
			return FILE_ERR;
		}
		iter->set_file_des(file_fd);
	}
	char s_command[MAX_RESPONSE];
	memset(s_command,0,MAX_RESPONSE);
	sprintf(s_command, "%s %d",command,cached);
	pass_server(sockfd, s_command);
	char response[MAX_RESPONSE];
	memset(response, 0, MAX_RESPONSE);
	recv_server(sockfd, response, MAX_RESPONSE);
	if(strncmp(response, GENERAL_FAIL, strlen(GENERAL_FAIL))==0){
		fprintf(stderr,"[ERROR] Server Failure\n");
		return FILE_ERR;	
	}else if(strncmp(response, NO_TRANSMISSION, strlen(NO_TRANSMISSION))==0){
		printf("No need of synchronization\n");
		return 0;	
	}else if(strncmp(response, LOCK_MES, strlen(LOCK_MES))==0){
		printf("File Locked\n");
		return 0;
	}else if(strncmp(response, NEED_TRANSMISSION, strlen(NEED_TRANSMISSION))==0){
		pass_server(sockfd, GENERAL_OK);
		if(recv_file(sockfd, file_name)!=0){
			fprintf(stderr,"[ERROR] File transmssion error\n");
			return FILE_ERR;
		}
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  create_file
 *  Description:  create a file and synchronize with the server
 * =====================================================================================
 */
int create_file(int sockfd, char* command){
	char file_name[MAX_NAME];
	if(sscanf(command,"%s %s",dummy, file_name)!=2){
		fprintf(stderr, "[ERROR] Format error\n");
		return FORMAT_ERR;
	}
	if(find_file(file_name)!=file_list.end()){
		printf("File %s exists\n", file_name);
		return 1;
	}
	string file_path = client_dir + file_name;
	int file_fd = open(file_path.c_str(), O_RDWR|O_APPEND|O_CREAT, RWRWRW);	
	if(file_fd == -1){
		fprintf(stderr,"file %s open error\n", file_name);
		return FILE_ERR;
	}
	close(file_fd);
	int file_uid = __sync_fetch_and_add(&max_file_uid,1);
	file_node new_node(file_uid, file_name);
	file_list.push_back(new_node);
	pass_server(sockfd, command);
	char response[MAX_RESPONSE];
	memset(response,0,MAX_RESPONSE);
	recv_server(sockfd,response,MAX_RESPONSE);
	if(strncmp(response,GENERAL_SUCCESS,strlen(GENERAL_SUCCESS))==0){
		printf("Create file %s succeeded\n",file_name);
	}else{
		printf("Create file %s failed\n",file_name);
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  recv_file
 *  Description:  receive a file from the server
 * =====================================================================================
 */
int recv_file(int sockfd, char* file_name){
	char response[MAX_RESPONSE];
	printf("Waiting for file transmission start\n");
	if(recv_server(sockfd, response, MAX_RESPONSE) < 0){
		fprintf(stderr, "[ERROR] read from server error\n");
		return FILE_ERR;
	}
	if(strncasecmp(response, TRANS_FILE_START, strlen(TRANS_FILE_START))!=0){
		fprintf(stderr, "File %s received error\n",file_name);
		return FILE_ERR;
	}
	long int file_length = 0;
	if(sscanf(response,"%s %ld", dummy, &file_length)!=2){
		return FILE_ERR;
	}
	pass_server(sockfd, (TRANS_FILE_START_ACK));
	string file_path = client_dir + file_name;
	FILE* fp = fopen(file_path.c_str(), "w+");	
	if(fp == NULL){
		fprintf(stderr,"[ERROR] file %s open error\n", file_name);
		return FILE_ERR;
	}
	if(file_length>0){
		char buffer[MAX_BUFF];
		memset(buffer, 0, sizeof(buffer));
		int length = 0;
		long total_read_byte = 0;
		while((length = read(sockfd, buffer, MAX_BUFF)) != 0){
			total_read_byte += length;
			printf("[RECV] buffer = %s\n", buffer);
			if(length<0){
				fprintf(stderr,"[ERROR] recv file %s error\n",file_name);
				break;
			}
			if(fwrite(buffer, sizeof(char), strlen(buffer), fp)!=strlen(buffer)){
				fprintf(stderr, "[ERROR ]File Write Error\n");
			}
			memset(buffer,0, sizeof(buffer));
			if(total_read_byte >= file_length){
				break;
			}
		}
	}
	printf("File %s received\n", file_name);
	fflush(fp);
	fclose(fp);
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  read_file
 *  Description:  read the file locally
 * =====================================================================================
 */
int read_file(int sockfd, char* command){
	//dump_file_list();
	char file_name[MAX_NAME];
	memset(file_name, 0, sizeof(file_name));
	if(sscanf(command,"read %s", file_name)==0||strlen(file_name)==0){
		fprintf(stderr,"[ERROR] format error\n");
		return FORMAT_ERR;
	}
	//string file_path = client_dir + file_name;
	vector<file_node>::const_iterator iter = find_file(file_name);
	if(iter == file_list.end()||iter->get_file_des() == -1){
		fprintf(stderr,"File %s not cached or opened\n",file_name);
		return FILE_ERR;
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
	fflush(fp);
	//fclose(fp);
	printf("Read file %s succeeded\n", file_name);
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  write
 *  Description:  write a file locally
 * =====================================================================================
 */
int write_file(int sockfd, char* command){
	char file_name[MAX_NAME];
	char content[MAX_BUFF];
	memset(file_name, 0, sizeof(file_name));
	memset(content, 0, sizeof(content));
	if(sscanf(command, "%s %s %s", dummy, file_name, content)!=3){
		fprintf(stderr, "[ERROR] Format error\n");
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = find_file(file_name);
	if(iter == file_list.end()||iter->get_file_des()==-1){
		fprintf(stderr,"File %s not cached or opened\n",file_name);
		return FILE_ERR;
	}
	FILE* fp  = fdopen(iter->get_file_des(),"a+");
	if(fp == NULL){
		fprintf(stderr,"[ERROR] file %s open error\n", file_name);
		return FILE_ERR;
	}
	if(fputs(content,fp )==EOF){
		fprintf(stderr,"[ERROR] file %s write error\n", file_name);
		return FILE_ERR;
	}
	fflush(fp);
	printf("File %s write finished\n", file_name);
	//fclose(fp);
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  close_file
 *  Description:  close a file locally. If the file is latest, store to the server. If out-of-date, synchronize with the server
 * =====================================================================================
 */
int close_file(int sockfd, char* command){
	char file_name[MAX_NAME];
	if(sscanf(command,"%s %s", dummy, file_name)!=2){
		fprintf(stderr, "[ERROR] Format error\n");
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = find_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr,"[ERROR] No such a file: %s\n", file_name);
		return FILE_ERR;
	}
	if(iter->get_file_des() == -1){
		fprintf(stderr,"[ERROR] File %s not open\n", file_name);
		return FILE_ERR;
	}
	pass_server(sockfd, command);
	char response[MAX_RESPONSE];
	char recv_md5[MAX_RESPONSE];
	memset(recv_md5, 0 ,MAX_RESPONSE);
	memset(response, 0, MAX_RESPONSE);
	recv_server(sockfd, response, MAX_RESPONSE);
	string file_path = client_dir+file_name;
	if(strncmp(response,GENERAL_FAIL, strlen(GENERAL_FAIL))==0){
		fprintf(stderr, "File %s Close Error\n", file_name);
		return FILE_ERR;
	}else if(strncmp(response,LOCK_MES,strlen(LOCK_MES))==0){
		fprintf(stderr, "File %s Lock Error\n", file_name);
		return FILE_ERR;
	}else if(strncmp(response,CLIENT_NEED_SYNC, strlen(CLIENT_NEED_SYNC))==0){
		pass_server(sockfd, GENERAL_OK);
		recv_file(sockfd,file_name);
		printf("File %s received\n", file_name);
		return 0;
	}else if(strncmp(response, FILE_STATUS, strlen(FILE_STATUS))==0){
		sscanf(response, "%s %s", dummy, recv_md5);
		MD5 md5;
		md5.reset();
		ifstream is(file_path.c_str());
		md5.update(is);
		string s_md5 = md5.toString().substr(0,6);
		cout << "Local MD5 = "<<s_md5<<endl;
		if(strcmp(recv_md5,s_md5.c_str())==0){
			printf("File %s consistent\n", file_name);
			pass_server(sockfd, FILE_CONSISTENT);
			return 0;
		}else{
			printf("File %s inconsistent\n", file_name);
			pass_server_file(sockfd, iter->get_file_des());
		}
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  pass_server_file
 *  Description:  store a file to server
 * =====================================================================================
 */
int pass_server_file(int sockfd,int file_fd){
	//dump_file_list();
	char response[MAX_RESPONSE];
	FILE* fp = fdopen(file_fd, "r+");
	if(fp == NULL){
		fprintf(stderr,"[ERROR] file open error, descriptor: %d\n", file_fd);
		return FILE_ERR;
	}
	//reset the file pointer to the beginning
	rewind(fp);
	struct stat file_stat;
	fstat(file_fd, &file_stat);
	long int file_length = file_stat.st_size;
	char trans_msg[MAX_RESPONSE];
	memset(trans_msg,0,sizeof(trans_msg));
	sprintf(trans_msg, "%s %ld", TRANS_FILE_START, file_length);
	if(pass_server(sockfd, trans_msg)!=0){
		fprintf(stderr,"File Transmission Error\n");
		return FILE_ERR;
	}
	printf("Waiting for ACK\n");
	if(recv_server(sockfd, response, MAX_RESPONSE)!=0||strncmp(response, TRANS_FILE_START_ACK, strlen(TRANS_FILE_START_ACK)!=0)){
		fprintf(stderr, "[ERROR] Transmission file failed\n");
		return FILE_ERR;
	}
	if(file_length>0){
		printf("File Transmission Start\n");
		char buffer[MAX_BUFF];
		memset(buffer, 0, sizeof(buffer));
		int read_byte = 0;
		long int total_byte = 0;
		while((read_byte = fread(buffer, sizeof(char), MAX_BUFF, fp))!=0){
			if(write(sockfd, buffer, strlen(buffer))<0){
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
		printf("Read byte = %d\n",read_byte);
		printf("File Transmission Finished\n");
	}
	fflush(fp);
	fclose(fp);
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delete_file
 *  Description:  delete the local file and remove the callback promise
 * =====================================================================================
 */
int delete_file(int sockfd, char* command){
	char file_name[MAX_NAME];
	memset(file_name,0, MAX_NAME);
	if(sscanf(command, "%s %s", dummy, file_name)!=2){
		fprintf(stderr, "[ERROR] Format error\n");
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = find_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr, "No such a file: %s\n",file_name);
		return FILE_ERR;
	}
	file_list.erase(iter);
	string file_path = client_dir+file_name;
	if(unlink(file_path.c_str())!=0){
		fprintf(stderr, "[ERROR] Delete file %s error\n", file_name);
		return FILE_ERR;
	}
	char newcommnad[MAX_RESPONSE];
	memset(newcommnad,0,MAX_RESPONSE);
	sprintf(newcommnad, "removecallback %s", file_name);
	remove_callback(sockfd, newcommnad);
	printf("Delete file %s succeeded\n", file_name);
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  status_file
 *  Description:  show the status of a file
 * =====================================================================================
 */
int status_file(int sockfd, char* command){
	char file_name[MAX_NAME];
	memset(file_name,0, MAX_NAME);
	if(sscanf(command,"%s %s",dummy, file_name)!=2){
		fprintf(stderr, "Format error\n");
		return FORMAT_ERR;
	}
	vector<file_node>::iterator iter = find_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr, "No such file %s\n", file_name);
		return FILE_ERR;
	}
	pass_server(sockfd,command);
	char response[MAX_RESPONSE];
	memset(response,0,MAX_RESPONSE);
	recv_server(sockfd, response,MAX_RESPONSE);
	if(strncmp(response,NO_SUCH_FILE,strlen(NO_SUCH_FILE))==0){
		fprintf(stderr, "No such file %s at server\n", file_name);
		return FILE_ERR;
	}else{
		cout << response << endl;
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  set_lock
 *  Description:  set a lock (shared or exclusive) to a file
 * =====================================================================================
 */
int set_lock(int sockfd, char* command){
	char lock_type[MAX_RESPONSE];
	char file_name[MAX_NAME];
	memset(file_name,0,sizeof(file_name));
	memset(lock_type,0,sizeof(lock_type));
	if(sscanf(command,"%s %s %s",dummy, file_name, lock_type)!=3){
		fprintf(stderr, "[ERROR] Format error\n");
		return FORMAT_ERR;
	}
	if(strcmp(lock_type,SHARED_LOCK)!=0&&strcmp(lock_type,EXCLUSIVE_LOCK)!=0){
		fprintf(stderr, "[ERROR] No such lock\n");
		return LOCK_ERR;
	}
	vector<file_node>::iterator iter = find_file(file_name);
	if(iter == file_list.end()){
		fprintf(stderr, "No such file\n");
		return FILE_ERR;
	}
	pass_server(sockfd, command);
	char buffer[MAX_BUFF];
	recv_server(sockfd, buffer, MAX_BUFF);
	if(strncmp(buffer, LOCK_FAIL, strlen(LOCK_FAIL))==0){
		printf("Lock failed\n");
	}else if(strncmp(buffer, LOCK_SUCCESS, strlen(LOCK_SUCCESS))==0){
		printf("Lock succeeded\n");
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  unset_lock
 *  Description:  release the lock
 * =====================================================================================
 */
int unset_lock(int sockfd, char* command){
	pass_server(sockfd, command);
	char buffer[MAX_BUFF];
	recv_server(sockfd, buffer, MAX_BUFF);
	if(strncmp(buffer, GENERAL_FAIL, strlen(GENERAL_FAIL))==0){
		printf("Unsetlock failed\n");
	}else if(strncmp(buffer, GENERAL_SUCCESS, strlen(GENERAL_SUCCESS))){
		printf("Unsetlock succeeded\n");
	}
	return 0;
}

int remove_callback(int sockfd, char* command){
	pass_server(sockfd, command);
	char buffer[MAX_BUFF];
	recv_server(sockfd, buffer, MAX_BUFF);
	if(strncmp(buffer, GENERAL_FAIL, strlen(GENERAL_FAIL))==0){
		printf("RemoveCallback failed\n");
	}else if(strncmp(buffer, GENERAL_SUCCESS, strlen(GENERAL_SUCCESS))){
		printf("RemoveCallback succeeded\n");
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  dump_file
 *  Description:  print information about all files
 * =====================================================================================
 */
int dump_file(int sockfd, char* command){
	dump_file_list();
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  add_callback
 *  Description:  add the client itself to the callback promise list
 * =====================================================================================
 */
int add_callback(int sockfd, char* command){
	printf("AddCallback\n");
	pass_server(sockfd, command);
	char buffer[MAX_BUFF];
	recv_server(sockfd, buffer, MAX_BUFF);
	if(strncmp(buffer, GENERAL_FAIL, strlen(GENERAL_FAIL))==0){
		printf("AddCallback failed\n");
	}else if(strncmp(buffer, GENERAL_SUCCESS, strlen(GENERAL_SUCCESS))){
		printf("AddCallback succeeded\n");
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  pass_server
 *  Description:  pass the instruction to server(encrypted)
 * =====================================================================================
 */
int pass_server(int sockfd, const char * command){
	printf("[SEND] %s\n", command);
	const char* encrypted = encrypt(command, ENCRYPT_KEY).c_str();
	if(write(sockfd, encrypted, strlen(encrypted)) == -1){
		perror("[ERROR] write error");
		return SOCKET_ERR;
	}
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  quit
 *  Description:  quit the client
 * =====================================================================================
 */
int quit(int sockfd, char* command){
	pass_server(sockfd, command);
	char recv_buf[MAX_BUFF];
	if(recv_server(sockfd,recv_buf, MAX_BUFF)!=0){
		fprintf(stderr, "Receive error\n");
		exit(1);
	}
	if(strncmp(recv_buf, "Quit", 4) == 0){
		printf("Exited\n");
	}
	exit(0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  recv_server
 *  Description:  receive a file from server
 * =====================================================================================
 */
int recv_server(int sockfd, char* buffer, int size){
	memset(buffer, 0, size);
	if(read(sockfd, buffer, size)<0){
		fprintf(stderr, "[ERROR] read from server failed\n");
		return FILE_ERR;	
	}
	const char * plain = decrypt(buffer, ENCRYPT_KEY).c_str();
	memset(buffer,0,size);
	strncpy(buffer, plain,strlen(plain));
	printf("[RECV] %s\n", plain);
	return 0;
}
