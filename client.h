/*
 * =====================================================================================
 *
 *       Filename:  client.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/06/14 00:25:28
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef  CLIENT_H_INC
#define  CLIENT_H_INC
#define COMMAND_NUM 13
using namespace std;
inline size_t min(size_t a, size_t b){
	return a<b?a:b;
}
class file_node {
	friend ostream& operator<<(ostream&, const file_node&);
	friend class File_equ;
	friend class File_equ_str;
	private:
	int file_uid;
	char file_name[MAX_NAME];
	int file_des;
	int callback;
	public:
	file_node():file_uid(0), file_des(-1), callback(1){}
	file_node(int uid, char *name, int des = -1, int cb = 1){
		memset(file_name,0,sizeof(file_name));
		file_uid = uid;
		strncpy(file_name, name, min(MAX_NAME,strlen(name)));
		file_des = des;
		callback = cb;
	}
	~file_node(){
		memset(file_name,0,MAX_NAME);
		file_uid = -1;
		file_des = -1;
		callback = 0;
	}
	int get_file_des() const{return file_des;}
	int get_file_uid() const{return file_uid;}
	int get_callback(){return callback;}
	string get_file_name() const{return file_name;}
	void set_file_des(int des) {file_des = des;}
	void set_callback(int cb) {callback = cb;}
};				/* ----------  end of struct file_node  ---------- */

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
	os << "file_uid = " << file.file_uid << " file_name = "<<file.file_name<<" file_des = "<<file.file_des<<" callback = "<<file.callback<<endl;
	return os;
}

int pass_server(int sockfd, const char* command);
int create_file(int sockfd, char* command);
int open_file(int sockfd, char* command);
int read_file(int sockfd, char* command);
int write_file(int sockfd, char* command);
int close_file(int sockfd, char* command);
int delete_file(int sockfd, char* command);
int status_file(int sockfd, char* command);
int set_lock(int sockfd, char* command);
int unset_lock(int sockfd, char* command);
int remove_callback(int sockfd, char* command);
int add_callback(int sockfd, char* command);
int dump_file(int sockfd, char* command);
int quit(int sockfd, char* command);

struct s_command{
	string name;
	int (*func)(int sockfd, char *command);
}command_list[] = {
	{"create", create_file},
	{"open", open_file},
	{"read", read_file},
	{"write", write_file},
	{"close", close_file},
	{"delete", delete_file},
	{"setlock", set_lock},
	{"unsetlock", unset_lock},
	{"removecallback", remove_callback},
	{"addcallback", add_callback},
	{"status", status_file},
	{"dump", dump_file},
	{"quit", quit},
};

int recv_file(int sockfd, char* file_name);
int echo_command(int sockfd, char * command);
void dump_file_list(void);
int recv_server(int sockfd, char* buffer, int size);
int pass_server_file(int sockfd_fd,int file_fd);
#endif   /* ----- #ifndef CLIENT_H_INC  ----- */
