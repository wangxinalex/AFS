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
 *       Compiler:  g++
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
	~client(){
		client_id = -1;
		sock_fd = -1;
	}
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
	int lock_owner;
	enum lock_t lock;
	pthread_rwlock_t promise_lock;
	pthread_rwlock_t invalid_lock;

	public:
	vector<int> promise_list;
	vector<int> invalid_list;
	file_node():file_uid(0), file_des(-1),lock_owner(-1), lock(no_lock){}
	file_node(int uid, char *name, int des = -1, int owner = -1, enum lock_t loc = no_lock){
		memset(file_name,0,sizeof(file_name));
		file_uid = uid;
		strncpy(file_name, name, min(MAX_NAME,strlen(name)));
		file_des = des;
		lock_owner = owner;
		lock = loc;
		pthread_rwlock_init(&promise_lock,NULL);
		pthread_rwlock_init(&invalid_lock,NULL);
	}
	~file_node(){
		pthread_rwlock_destroy(&promise_lock);
		pthread_rwlock_destroy(&invalid_lock);
	}
	void promise_rdlock(){pthread_rwlock_rdlock(&promise_lock);}
	void invalid_rdlock(){pthread_rwlock_rdlock(&invalid_lock);}
	void promise_unlock(){pthread_rwlock_unlock(&promise_lock);}
	void invalid_unlock(){pthread_rwlock_unlock(&invalid_lock);}
	int get_file_des() const{return file_des;}
	int get_file_uid() const{return file_uid;}
	int get_lock_owner() const{return lock_owner;}
	string get_file_name() const{return file_name;}
	lock_t get_file_lock() const{return lock;}
	void set_file_des(int val){file_des = val;}
	void set_lock_owner(int owner){lock_owner = owner;}
	void set_file_lock(enum lock_t loc){lock = loc;}
	int add_invalid_id(int id);
	int delete_invalid_id(int id);
	int add_promise_id(int id);
	int only_promise_id(int id);
	int delete_promise_id(int id);
	int exist_promise_id(int id);
	int exist_invalid_id(int id);
	string to_string();
};	
string file_node::to_string(){
	ostringstream oss;
	oss << "file_uid = " << file_uid << " file_name = "<<file_name<<" lock_owner = "<<lock_owner<<" lock_type = "<<lock<<" promise_list = [";
	ostringstream oss_list;
	for(vector<int>::const_iterator iter = promise_list.begin();iter!= promise_list.end();iter++){
		oss_list<<*iter<<",";
	}
	string s_list = oss_list.str();
	if(s_list.size()!=0){
		s_list = s_list.substr(0,s_list.size()-1);
	}
	oss<<s_list<<"] invalid_list = [";
	oss_list.str("");
	s_list.clear();
	for(vector<int>::const_iterator iter = invalid_list.begin();iter!= invalid_list.end();iter++){
		oss_list<<*iter<<",";
	}
	s_list = oss_list.str();
	if(s_list.size()!=0){
		s_list = s_list.substr(0,s_list.size()-1);
	}
	oss<<s_list<<"]"<<endl;
	return oss.str();
}
int file_node::add_invalid_id(int id){
	pthread_rwlock_wrlock(&invalid_lock);
	if(find(this->invalid_list.begin(), this->invalid_list.end(),id)==this->invalid_list.end()){
		this->invalid_list.push_back(id);
	}
	pthread_rwlock_unlock(&invalid_lock);
	return 0;
}
int file_node::delete_invalid_id(int id){
	pthread_rwlock_wrlock(&invalid_lock);
	vector<int>::iterator iter;
	if((iter = find(this->invalid_list.begin(), this->invalid_list.end(),id))!=this->invalid_list.end()){
		this->invalid_list.erase(iter);
	}
	pthread_rwlock_unlock(&invalid_lock);
	return 0;
}

int file_node::add_promise_id(int id){
	pthread_rwlock_wrlock(&promise_lock);
	if(find(this->promise_list.begin(), this->promise_list.end(),id)==this->promise_list.end()){
		this->promise_list.push_back(id);
	}
	pthread_rwlock_unlock(&promise_lock);
	return 0;
}
int file_node::delete_promise_id(int id){
	pthread_rwlock_wrlock(&promise_lock);
	vector<int>::iterator iter;
	if((iter = find(this->promise_list.begin(), this->promise_list.end(),id))!=this->promise_list.end()){
		this->promise_list.erase(iter);
	}
	pthread_rwlock_unlock(&promise_lock);
	return 0;
}
int file_node::only_promise_id(int id){
	promise_rdlock();
	int only_one = this->promise_list.size()==1;
	int exist = find(this->promise_list.begin(), this->promise_list.end(),id)!=this->promise_list.end();
	promise_unlock();
	return exist&&only_one;
}
int file_node::exist_promise_id(int id){
	promise_rdlock();
	int exist = find(this->promise_list.begin(), this->promise_list.end(),id)!=this->promise_list.end();
	promise_unlock();
	return exist;
}

int file_node::exist_invalid_id(int id){
	invalid_rdlock();
	int exist = find(this->invalid_list.begin(), this->invalid_list.end(),id)!=this->invalid_list.end();
	invalid_unlock();
	return exist;
}

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
	os << "file_uid = " << file.file_uid << " file_name = "<<file.file_name<<" file_des = "<<file.file_des<<" lock_owner = "<<file.lock_owner<<" lock_type = "<<file.lock<<" promise_list = [";
	ostringstream oss;
	for(vector<int>::const_iterator iter = file.promise_list.begin();iter!= file.promise_list.end();iter++){
		oss<<*iter<<",";
	}
	string s_list = oss.str();
	if(s_list.size()!=0){
		s_list = s_list.substr(0,s_list.size()-1);
	}
	os<<s_list<<"] invalid_list = [";
	oss.str("");
	for(vector<int>::const_iterator iter = file.invalid_list.begin();iter!= file.invalid_list.end();iter++){
		oss<<*iter<<",";
	}
	s_list.clear();
	s_list = oss.str();
	if(s_list.size()!=0){
		s_list = s_list.substr(0,s_list.size()-1);
	}
	os<<s_list<<"]"<<endl;
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
int close_file(int client_fd, char * command);
int status_file(int client_fd,char * command);
int setlock_file(int client_fd,char * command);
int unsetlock_file(int client_fd,char * command);
int removecallback_file(int client_fd,char * command);
int addcallback_file(int client_fd,char * command);
int quit(int client_fd, char* command);

struct s_command{
	string name;
	int (*func)(int client_fd,char *command);
}command_list[] = {
	{"create", create_file},
	{"open", open_file},
	{"close", close_file},
	{"status", status_file},
	{"setlock", setlock_file},
	{"unsetlock", unsetlock_file},
	{"removecallback", removecallback_file},
	{"addcallback", addcallback_file},
	{"quit", quit},
};

int add_client(int sockfd);
void dump_client_list(void);
int echo_command(int client_id, char * command);
vector<client>::iterator get_client(int client_id);
void *handle(void *p);
void dump_file_list();
vector<file_node>::iterator find_file(int uid);
vector<file_node>::iterator find_file(char *name);
int add_file_list(int client_fd, char* file_name, int file_fd);
int add_invalid_id(int id, vector<file_node>::iterator& fiter);
int delete_invalid_id(int id, vector<file_node>::iterator& fiter);
template<class T> int exist(vector<T>& vec, T val);
int recv_client_file(int client_id, char* file_name, long file_length);
#endif   /* ----- #ifndef SERVER_H_INC  ----- */
