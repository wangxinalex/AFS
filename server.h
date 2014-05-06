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
#ifndef  SERVER_H_INC
#define  SERVER_H_INC
#define COMMAND_NUM 7
using namespace std;

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
#endif   /* ----- #ifndef SERVER_H_INC  ----- */
