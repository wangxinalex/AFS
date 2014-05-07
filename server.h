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
#define COMMAND_NUM 9
using namespace std;

inline size_t min(size_t a, size_t b){
	return a<b?a:b;
}

class client {
	friend ostream& operator<<(ostream &os, const client& obj);
	friend class Client_equal;
private:
	int client_id;
	int sock_fd;
public:
	client(int clientid, int sockfd){
		client_id = clientid;
		sock_fd = sockfd;
	}
	int get_client_id() const{return client_id;}
	int get_sock_fd() const{return sock_fd;}
};				/* ----------  end of struct client  ---------- */

enum lock_t{no_lock, shared_lock, exclusive_lock};

class file_node {
	friend ostream& operator<<(ostream&, const file_node&);
	friend class File_equ;
	friend class File_equ_str;
	private:
	int file_uid;
	char file_name[MAX_NAME];
	int file_des;
	enum lock_t lock;
	public:
	vector<int> promise_list;
	file_node():file_uid(0), file_des(-1), lock(no_lock){}
	file_node(int uid, char *name, int des = -1, enum lock_t loc = no_lock){
		memset(file_name,0,sizeof(file_name));
		file_uid = uid;
		strncpy(file_name, name, min(MAX_NAME,strlen(name)));
		file_des = des;
		lock = loc;
	}
	int get_file_des() const{return file_des;}
	int get_file_uid() const{return file_uid;}
	string get_file_name() const{return file_name;}
	lock_t get_file_lock() const{return lock;}
	void set_file_des(int val){file_des = val;}
	void set_file_lock(enum lock_t loc){lock = loc;}
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
	os << "file_uid = " << file.file_uid << " file_name = "<<file.file_name<<" lock_type = "<<file.lock<<" promise_list = [";
	for(vector<int>::const_iterator iter = file.promise_list.begin();iter!= file.promise_list.end();iter++){
		os<<*iter<<",";
	}
	os<<"]"<<endl;
	return os;
}
ostream& operator<<(ostream& os, const client& obj){
	os<<"client_id = "<<obj.client_id<<" sock_fd = "<<obj.sock_fd<<endl;
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
int setlock_file(int client_fd,char * command);
int unsetlock_file(int client_fd,char * command);
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
	{"setlock", setlock_file},
	{"unsetlock", unsetlock_file},
	{"quit", quit},
};

int add_client(int sockfd);
void dump_client_list(void);
int echo_command(int client_id, char * command);
vector<client>::const_iterator get_client(int client_id);
void *handle(void *p);
void dump_file_list();
vector<file_node>::iterator get_file(int uid);
vector<file_node>::iterator get_file(char *name);
#endif   /* ----- #ifndef SERVER_H_INC  ----- */
