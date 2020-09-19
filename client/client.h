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
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
//#include "fcntl-linux.h"
#include <limits.h>
#include <termio.h>
#include <sys/sendfile.h>
#include "wrang.h"
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
#define BUF 2048


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
//#define VIEW_FRIEND_STATUS_APPLY 32

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
#define UNBLACK 1
#define BLACK 0

#define OWNER 1
#define ADMIN 2
#define COMMON 3

pthread_mutex_t mutex_gmb;
pthread_cond_t cond_gmb;

pthread_mutex_t mutex_msg;
pthread_cond_t cond_msg;

pthread_mutex_t mutex_login;
pthread_cond_t cond_login;

pthread_mutex_t mutex_show;
pthread_cond_t cond_show;

pthread_mutex_t mutex_add_grp;
pthread_cond_t cond_add_grp;

pthread_mutex_t mutex_gchat;
pthread_cond_t cond_gchat;

pthread_mutex_t mutex_gcord;
pthread_cond_t cond_gcord;

int cfd;

int flag_login=0;

typedef struct message
{
    int flag;
    int id;
    char messsge[256];
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
    //struct box* next;
}box_t;

typedef struct box_node
{
    box_t data;
    struct box_node *next;
    struct box_node *prev;
}box_node_t,*box_list_t;
box_list_t head;
box_list_t fhead;



typedef struct relation_info
{
    int flag;
    int sender;
    int recver;
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




typedef struct file
{
    int flag;
    int sender;
    int recver;
    int file_size;
    char file_name[100];
    char data[800];
}file_t;
int len;



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

int username;
int nums=0;
void Login_menu();
void Menu();
void Friend_menu();
void Group_menu();


void Register();
void Register_apply(char* buf);
//void Register_error_apply(char* buf);

void Login();
void Login_apply(char* buf);

void Exit();
void Exit_apply(char* buf);

void Add_friend();
void Add_friend_apply(char* buf);
void Friend_box();
void Add_friend_accept_appy(char* buf);

void Del_friend();
void Del_friend_apply(char* buf);

void Shield_friend();
void Shield_friend_apply(char* buf);

void Unshield_friend();
void Unshield_friend_apply(char* buf);

void View_friend();
void View_friend_list_apply(char* buf);

void Private_chat();
void Private_chat_apply(char* buf);

void View_chat_history();
void View_chat_history_apply(char*  buf);


void Create_group();
void Create_group_apply(char* buf);

void Add_group();
void Add_group_apply(char* buf);

void Group_box();
void Group_apply(char* buf);
void Add_group_accept_apply(char* buf);

void Withdraw_group();
void Withdraw_group_apply(char* buf);
void Del_group_apply(char* buf);

void View_add_group();
void View_add_group_apply(char* buf);


void View_group_member();
void View_group_member_apply(char* buf);


void Group_chat();
void Group_chat_apply(char* buf);

void View_group_record();
void View_group_record_apply(char* buf);

void Set_group_admin();
void Set_group_admin_apply(char* buf);

void Kick();
void Kick_apply(char* buf);


void File_menu();

//void Upload();
void Send_file();
//void* Send_file(void *arg);

void Download();
//void* Recv_file(void *arg);
void Recv_file(char* buf);
void Recv_apply(char* buf);


void Send_message(int flag,char* buf);




void display(char* str);
void my_err(const char* err_string,int line);
char* Get_string(char* buf,int len);
//char getch();
int getch();
void Clear_buffer();
int Get_choice(char *choice_t);

void Init_socket();
void *Recv_pack(void* arg);
void Turn_worker_thread();

