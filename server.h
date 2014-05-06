/*
 * =====================================================================================
 *
 *       Filename:  server.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/05/14 19:01:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef SERVER_H_INC
#define SERVER_H_INC
#define COMMAND_NUM 7
#define	MAX_NAME 30			/*  */
using namespace std;

inline size_t min(size_t a, size_t b){
	return a<b?a:b;
}

class client {
	friend ostream& operator<<(ostream &os, const client& obj);
	friend class Client_equal;
public:
	client(int clientid, int sockfd){
		client_id = clientid;
		sock_fd = sockfd;
	}
	int get_client_id(){return client_id;}
	int get_sock_fd(){return sock_fd;}
private:
	int client_id;
	int sock_fd;
};				/* ----------  end of struct client  ---------- */

class file_node {
	friend ostream& operator<<(ostream&, const file_node&);
	friend class File_equ;
	friend class File_equ_str;
	private:
	int file_uid;
	char file_name[MAX_NAME];
	int file_des;
	public:
	vector<int> promise_list;
	file_node():file_uid(0), file_des(-1){}
	file_node(int uid, char *name, int des = -1){
		memset(file_name,0,sizeof(file_name));
		file_uid = uid;
		strncpy(file_name, name, min(MAX_NAME,strlen(name)));
		file_des = des;
	}
	int get_file_des(){return file_des;}
	int get_file_uid(){return file_uid;}
	char* get_file_name(){return file_name;}
};	

class File_equ{
	private:
		int equ;
	public:
		File_equ(int val):equ(val){}
		bool operator()(const file_node& file){
			return file.file_uid == equ;
		}
};

class File_equ_str{
	private:
		char str[MAX_NAME];
	public:
		File_equ_str(char *s){strcpy(str, s);}
		bool operator()(const file_node& file){
			return strcmp(file.file_name, str)==0;
		}
};

ostream& operator<<(ostream& os, const file_node& file){
	os << "file_uid = " << file.file_uid << " file_name = "<<file.file_name<<" promise_list = [";
	for(vector<int>::const_iterator iter = file.promise_list.begin();iter!= file.promise_list.end();iter++){
		os<<*iter<<",";
	}
	os<<"]"<<endl;
	return os;
}
ostream& operator<<(ostream& os, const client& obj){
	os<<"client_id = "<<obj.client_id<<" sock_fd = "<<obj.client_id<<endl;
	return  os;
}

class Client_equal{
	public:
		Client_equal(size_t val = 0):equ(val){}
		bool operator()(const client& c){
			return c.client_id == equ;
		}
	private:
		int equ;
};

int create_file(int client_fd, char * command);
int open_file(int client_fd, char * command);
int read_file(int client_fd, char * command);
int close_file(int client_fd, char * command);
int delete_file(int client_fd, char * command);
int status_file(int client_fd,char * command);
int quit(int client_fd, char* command);

struct s_command{
	string name;
	int (*func)(int client_fd,char *command);
}command_list[] = {
	{"create", create_file},
	{"open", open_file},
	{"read", read_file},
	{"close", close_file},
	{"delete", delete_file},
	{"status", status_file},
	{"quit", quit},
};

int add_client(int sockfd);
void dump_list(void);
int echo_command(int client_id, char * command);
vector<client>::const_iterator get_client(int client_id);
void *handle(void *p);
void dump_file_list();
#endif   /* ----- #ifndef SERVER_H_INC  ----- */
