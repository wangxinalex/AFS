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
 *       Compiler:  gcc
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef  CLIENT_H_INC
#define  CLIENT_H_INC
#define MAX_NAME 30
#define COMMAND_NUM 11
#define FORMAT_ERR 1
#define SOCKET_ERR 2
#define FILE_ERR 3
inline size_t min(size_t a, size_t b){
	return a<b?a:b;
}
using namespace std;
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
	file_node():file_uid(0), file_des(-1), callback(0){}
	file_node(int uid, char *name, int des = -1, int cb = 0){
		memset(file_name,0,sizeof(file_name));
		file_uid = uid;
		strncpy(file_name, name, min(MAX_NAME,strlen(name)));
		file_des = des;
		callback = cb;
	}
	int get_file_des() const{return file_des;}
	int get_file_uid() const{return file_uid;}
	string get_file_name() const{return file_name;}
	void set_file_des(int des) {file_des = des;}
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

int pass_server(int sockfd, char* command);
int open_file(int sockfd, char* command);
int read_file(int sockfd, char* command);
int write_file(int sockfd, char* command);
int close_file(int sockfd, char* command);
int delete_file(int sockfd, char* command);
int status_file(int sockfd, char* command);
int quit(int sockfd, char* command);

struct s_command{
	string name;
	int (*func)(int sockfd, char *command);
}command_list[] = {
	{"create", pass_server},
	{"open", open_file},
	{"read", read_file},
	{"write", write_file},
	{"close", close_file},
	{"delete", delete_file},
	{"setlock", pass_server},
	{"unsetlock", pass_server},
	{"removecallback", pass_server},
	{"status", status_file},
	{"quit", quit},
};

int recv_file(int sockfd, char* command, char* file_name);
int echo_command(int sockfd, char * command);
void dump_file_list(void);
#endif   /* ----- #ifndef CLIENT_H_INC  ----- */
