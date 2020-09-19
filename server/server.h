#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <sys/sendfile.h>
#include "wrang.h"
//#include "prest.h"
#include "List.h"
#include "pthreadpool.h"




//#define SERV_ADDRESS "47.94.14.45"
//#define SERV_ADDRESS "127.0.0.1"
#define SERV_ADDRESS "192.168.30.185"
//#define SERV_ADDRESS "192.168.1.184"
#define SERV_PORT 8013

#define MAX 50
#define MAX_CHAR 300
#define SAVE 10

#define REGISTER 1
#define LOGIN 2
#define MODIFY 3

#define ADD_FRIEND 4
#define DEL_FRIEND 5
//#define QUERY_FRIEND 6
#define PRIVATE_CHAT 7
#define VIEW_FRIEND_LIST 8
//#define SHOW_FRIEND_STATUS 9
#define VIEW_CHAT_HISTORY 10
#define SHIELD 11
#define UNSHIELD 12
#define SHOW_FRIEND 13
//#define GET_FRIEND_STATUS 14

#define CREAT_GROUP 15
#define ADD_GROUP 16
#define DEL_GROUP 17
#define WITHDRAW_GROUP 18
#define KICK 19
#define SET_GROUP_ADMIN 20
#define VIEW_ADD_GROUP 21
#define VIEW_GROUP_MEMBER 22 
#define VIEW_GROUP_RECORD 23
#define SEND_FILE 24
#define GROUP_CHAT 25

#define ADD_FRIEND_APPLY 26
#define DEL_FRIEND_APPLY 27
#define PRIVATE_CHAT_APPLY 28
#define SHIELD_APPLY 29
#define UNSHIELD_APPLY 30
#define VIEW_FRIEND_LIST_APPLY 31
//#define SHOW_FRIEND_STATUS_APPLY 32

#define CREAT_GROUP_APPLY 33
#define ADD_GROUP_APPLY 34
#define DEL_GROUP_APPLY 35
#define WITHDRAW_GROUP_APPLY 36
#define SET_GROUP_ADMIN_APPLY 37
#define KICK_APPLY 38
#define VIEW_ADD_GROUP_APPLY 39
#define VIEW_GROUP_MEMBER_APPLY 40
#define MESSAGE_RECORD 41


#define REGISTER_APPLY 42
#define LOGIN_APPLY 43
#define PRINT_APPLY 44
#define ADD_FRIEND_ACCEPT 45
#define VIEW_CHAT_HISTORY_APPLY 46
#define ADD_FRIEND_ACCEPT_APPLY 47
#define GROUP_APPLY 48
#define ADD_GROUP_ACCEPT 49
#define ADD_GROUP_ACCEPT_APPLY 50
#define VIEW_GROUP_RECORD_APPLY 51
#define GROUP_CHAT_APPLY 52
#define RECV_FILE 53
//#define REGISTER_ERROR_APPLY 54
#define EXIT 54
//#define EXIT_APPLY 55
#define RECV_APPLY 55
#define UPLOAD 56
#define DOWNLOAD 57

#define DOWNLINE 0
#define ONLINE 1


#define OWNER 1
#define ADMIN 2
#define COMMON 3
#define ADOPTER 4

#define STRANGER 0
#define PAL 1
#define BLACK 2
#define UNBLACK 3

#define MAX_THREAD_NUM 10


typedef struct message
{
    int flag;
    int id;
    char message[256];
}message;




typedef struct apply_messgae
{
    int flag;
    int sender;
    int recver;
    int send_fd;
    int recv_fd;
    char message[256];
}apply_messgae;


typedef struct chat_message
{
    int flag;
    int sender;
    int recver;
    char message[256];
    //char time[30];
}Chat_message;

typedef struct box
{
    int flag;
    int sender;
    int recver;
    int send_fd;
    int recv_fd;
    char message[256];
}box_t;




typedef struct relation_info
{
    int flag;
    int send;
    int recv;
    int relation;
    char message[256];
}Relation_t;

typedef struct friend_info
{
    int flag;
    int send;
    int recv;
    int status;
    int relation;
    char name[MAX];
    char message[256];
}Friend_t;



typedef struct group_info
{
    int flag;
 
    int group_owner;
    int admin;
    char group_name[MAX];
}Group_t;


typedef struct group_leader
{
    int flag;
    int sender;
    int recver;
    int admin;
    char message[256];
}Group_leader;



//服务器保存用户信息结构体
typedef struct account
{
    int flag;
    int id;
    char name[MAX];
    char password[MAX];
    struct sockaddr_in useraddr;
  
    
    int online;      //1:开;0:关
    int connfd;      //链接套接字
}Account_t;         


int user_num;

typedef struct server_user
{
    int connfd;
    int id;
    char name[MAX];
    char password[MAX];
}server_user_t;


typedef struct server_user_node
{
    server_user_t data;
    struct server_user_node* next;
    struct server_user_node* prev;
}server_user_node_t,*server_list_t;

server_list_t list_ser;




typedef struct file
{
   
    int flag;
    int sender;
    int recver;
    int file_size;
    char file_name[100];
    char data[800];
}file_t;




int lfd;
int epfd;
int cfd;

void Init_socket();
void *Recv_pack(void* arg);
void Turn_worker_thread();
void *work(void* arg);

void Login(int fd,char* buf);
void Send_offline_apply(int fd,int recver);
void Send_offline_messgae(int fd,int recver);
void Register(int fd,char* buf);
void Exit(int fd,char* buf);

void Add_friend(int fd,char* buf);
void Add_friend_accept(int fd,char* buf);
void Del_friend(int fd,char* buf);


void Shield_friend(int fd,char* buf);
void Unshield_friend(int fd,char* buf);
//一起实现
//void Show_friend_status();
void View_friend_list(int fd,char* buf);

int Check_relationship(int fd,int send,int recv);
void Private_chat(int fd,char* buf);
void View_chat_history(int fd,char* buf);

void Create_group(int fd,char* buf);
void Add_group(int fd,char* buf);
void Add_group_accept(int fd,char* buf);
void Withdraw_group(int fd,char* buf);

//一起实现
void View_group_member(int fd,char* buf);
void View_add_group(int fd,char* buf);

void Group_chat(int fd,char* buf);
void View_group_record(int fd,char* buf);


void Set_group_admin(int fd,char* buf);
void Kick(int fd,char* buf);


int Check_relationship2(int fd,int send,int recv);
void Upload(int fd,char* buf);
void *Send_file(void *arg);

void Recv_file(int fd,char* buf);
void Download(int fd,char* buf);


void my_err(const char* err_string,int line);
void Connect_mysql();
void Close_mysql(MYSQL mysql);


int Get_connfd(int id);
void Send_pack(int fd,int flag,char* buf);
void Send_connfd_pack(int flag,int sender,int recver,char* buf);
void Send_pack_name(int flag ,int sender,int recver,char *buf);





void Mysql_with_error(MYSQL* mysql);
void Signal_close(int i);



void Add_node(int fd,int id,char* name);
void Send_register_pack(int fd,int flag,char* buf,int id);
int Get_status(int id);
char* Get_name(int id);

