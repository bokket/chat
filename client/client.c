#include "client.h"
#define SIZE 100
#define MAXLEN  80 //一行显示的最多字符数
#define BUFFSIZE 32
#define BUFMAX 1024

int g_leave_len=MAXLEN; //一行剩余长度，用于输出对齐
int g_maxlen;      //存放某目录下最长文件名的长度


void Print_menu();

int Check_data(char *num);//检查输入是否为数字
int Get_choice_int(char* str);
char *Time();
void Print_apply(char* buf);
int GetPassword(char *password);
int get_choice_int(int min, int max);

void Client_list();
void Display(char* pathname);
void Display_single(char* name);


void clear_stdin();
char* get_str(char* str,size_t len);






int main()
{
    Init_socket();
    

    List_Init(head,box_node_t);
    List_Init(fhead,box_node_t);
    pthread_mutex_init(&mutex_login,NULL);
    pthread_cond_init(&cond_login,NULL);

    pthread_mutex_init(&mutex_add_grp,NULL);
    pthread_cond_init(&cond_add_grp,NULL);

    pthread_mutex_init(&mutex_gchat,NULL);
    pthread_cond_init(&cond_gchat,NULL);

    pthread_mutex_init(&mutex_show,NULL);
    pthread_cond_init(&cond_show,NULL);

    pthread_mutex_init(&mutex_msg,NULL);
    pthread_cond_init(&cond_msg,NULL);

    pthread_mutex_init(&mutex_gcord,NULL);
    pthread_cond_init(&cond_gcord,NULL);

    pthread_mutex_init(&mutex_gmb,NULL);
    pthread_cond_init(&cond_gmb,NULL);


    Login_menu();
    //printf("开始收包\n");
    Turn_worker_thread();
    //Menu();

    close(cfd);
}
void Init_socket()
{
    
    
    printf("客户端启动\n");
    struct sockaddr_in serv_addr;


    cfd=Socket(AF_INET,SOCK_STREAM,0);

    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(SERV_PORT);
    inet_pton(AF_INET,SERV_ADDRESS,&serv_addr.sin_addr.s_addr);
    //serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    Connect(cfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

    Turn_worker_thread();

    printf("客户端启动成功!\n");

}

//密码回显
//返回值不包括'\0'
int GetPassword(char *password)
{
	char ch;
	int i=0;
    do
    {
        ch = getch();
        if(ch != '\n' && ch != '\r' && ch!=127)
        {
            password[i]=ch;
            printf("*");
            i++;
        }
        else if(((ch!='\n')|(ch!='\r'))&&(ch==127))
        {
            if(i>0)
            {
                i--;
                printf("\b \b");
            }
        }
    }while(ch!='\n'&&ch!='\r');
    password[i]='\0';
	return i;
}
void Register()
{
    Account_t register_t;
    
    char buf[BUFMAX];
    memset(buf,0,sizeof(buf));
    printf("用户名:");
    printf("[此用户名为您暂时的名字,并不是登录账号]\n");
    scanf("%s",register_t.name);
    //fgets(register_t.username,sizeof(register_t.username),stdin);
    printf("密码:");
    scanf("%s",register_t.password);
    //fgets(register_t.password,sizeof(register_t.password),stdin);
    register_t.flag=REGISTER;

    memcpy(buf,&register_t,sizeof(Account_t));

    printf("client flag:%d\n",register_t.flag);
    printf("client message:%s\n",register_t.password);
    printf("client send:%s\n",register_t.name);
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error!",__LINE__);
    }
}
void Register_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("--------%s\n",mes.messsge);
    int id=mes.id;
    printf("----等会输入账号时请输入系统给您的账号\n");
    printf("您的账号是:%d\n",mes.id);
    printf(">...\n");
    printf("正在返回登录界面\n");
    usleep(1000);
    Login_menu();
    //sleep(1);
    //getchar();
}
/*void Register_error_apply(char* buf)
{
    message mes;
    printf("-----%s\n",mes.messsge);
}*/
int Check_data(char *num)
{
    int i;
    for(i=0;num[i];i++) 
    {
        if(num[i]>'9' || num[i]<'0')//只要有非数字，就返回错误
        {
            printf("您的输入不为数字\n");
            return 0;
        }
    }

    if(i>100)//长度超过100位，返回错误
    {
        printf("超出数字长度\n");
        return 0;
    }
    return 1;
}
int Get_choice_int(char* str)
{
    while (1)
    {
        scanf("%s",str);
        if (Check_data(str)==0)
        {
            //printf("wraning------[[[不要输入之前注册的用户名]]]\n");
            printf("输入错误,请重新输入整数\n");
        }
        else
        {
            printf("输入正确\n");
            break;
        }
    }
    int choice;
    choice=atoi(str);
    return choice;
}
void Login()
{
    char buf[BUFMAX];
    Account_t account;
    account.flag=LOGIN;
    //printf("wraning------[[[不要输入之前注册的用户名]]]\n");
    printf("请输入账号:\n");
    //scanf("%d",&account.id);
    char str[SIZE];
    while (1)
    {
        scanf("%s",str);
        if (Check_data(str)==0)
        {
            printf("wraning------[[[不要输入之前注册的用户名]]]\n");
            printf("输入错误,请重新输入\n");
        }
        else
        {
            printf("账号输入正确\n");
            break;
        }
    }

    //printf("%d\n", atoi(str));
    account.id=atoi(str);

    printf("请输入密码:\n");
    scanf("%s",account.password);
    
    username=account.id;
    memcpy(buf,&account,sizeof(account));
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error!",__LINE__);
    }

    pthread_mutex_lock(&mutex_login);
    pthread_cond_wait(&cond_login,&mutex_login);
    if(flag_login==1)
        Menu();
    flag_login=0;
    pthread_mutex_unlock(&mutex_login);
}
void Login_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    pthread_mutex_lock(&mutex_login);
    if(strcmp(mes.messsge,"y")==0)
    {
        printf("登录成功\n");
        flag_login=1;
    }
    else if(strcmp(mes.messsge,"a")==0)
        printf("账号已登录\n");
    else if(strcmp(mes.messsge,"n")==0)
        printf("密码错误或账户不存在\n");
    /*else if 
    {
        printf("---%s\n",mes.message);
    }*/
    
    
    pthread_cond_signal(&cond_login);
    pthread_mutex_unlock(&mutex_login);
}
void Login_menu()
{

    int choice=-1;
   
    //int choice=-1;
    while(choice)
    {
        usleep(10000);
        //system("clear");
        printf("\t\t\033[44;34m\033[44;37m**************************\033[0m\n");
        printf("\t\t\033[1;34m*        1.注册          \033[1;34m*\033[0m \n");
        printf("\t\t\033[1;34m*        2.登录          \033[1;34m*\033[0m \n");
        printf("\t\t\033[1;34m*        0.退出          \033[1;34m*\033[0m \n");
        printf("\t\t\033[44;34m\033[44;37m**************************\033[0m\n");
        printf("\t\tchoice：");
        char choice_t[SIZE];
        choice=Get_choice_int(choice_t);
        //choice=Get_choice_int();
        //scanf("%d",&choice);
        //while(getchar()!='\n');
        //choice=Get_choice(choice_t);    
        switch(choice)
        {
            case 1:
                puts("注册");
                Register();
                break;
            case 2:
                puts("登录");
                Login();
                break;
            /*case 3:
                puts("找回密码");
                Modify_password();
                break;*/
            default:
                break;
        }
    }
}
void Exit()
{
    message mes;
    mes.flag=EXIT;
    mes.id=username;
    char mess[256];
    sprintf(mess,"用户[%d]退出登录\n",username);
    strcpy(mes.messsge,mess);

    char buf[BUFMAX];
    memcpy(buf,&mes,sizeof(mes));
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error",__LINE__);
    }
   // Login_menu();
}
/*void Exit_apply(char* buf)
{   
    pthread_mutex_lock(&mutex);
    message mes;
    printf("---%s\n",mes.messsge);
    pthread_cond_wait(&cond,&mutex);
    pthread_mutex_unlock(&mutex);

}*/
void *Recv_pack(void *arg)
{
    pthread_t apply_id;
    char buf[BUFMAX];    
    while(1)
    {
        int n=recv(cfd,buf,sizeof(buf),0);
        if(n<0)
        {
            my_err("recv error",__LINE__);
        }
        //printf("client recv:%s\n",buf);
        //pthread_mutex_lock(&mutex);
        else if(n==0)
        {
            printf("\n服务端已经停止了工作,请退出客户端\n");
            exit(0);
        }
        int flag;
        memcpy(&flag,buf,sizeof(int));
        //printf("client recv flag:%d\n",flag);
        switch (flag)
        {
            /*case EXIT_APPLY:
                pthread_mutex_lock(&mutex);
                Exit_apply(buf);
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&mutex);
                break;*/
            case PRINT_APPLY:
                Print_apply(buf);
                break;
            case REGISTER_APPLY:
                Register_apply(buf);
                break;
            /*case REGISTER_ERROR_APPLY:
                Register_error_apply(buf);
                break;*/
            case LOGIN_APPLY:
                Login_apply(buf);
                break;
            case ADD_FRIEND_APPLY:
                Add_friend_apply(buf);
                break;
            case ADD_FRIEND_ACCEPT_APPLY:
                Add_friend_accept_appy(buf);
            case DEL_FRIEND_APPLY:
                Del_friend_apply(buf);
                break;
            //一起实现
            //case SHOW_FRIEND_STATUS_APPLY:
            case VIEW_FRIEND_LIST_APPLY:
                View_friend_list_apply(buf);
                break;
            case SHIELD_APPLY:
                Shield_friend_apply(buf);
                break;
            case UNSHIELD_APPLY:
                Unshield_friend_apply(buf);
                break;
            case PRIVATE_CHAT_APPLY:
                Private_chat_apply(buf);
                break;
            case VIEW_CHAT_HISTORY_APPLY:
                View_chat_history_apply(buf);
                break;
            case CREAT_GROUP_APPLY:
                Create_group_apply(buf);
                break;
            case ADD_GROUP_APPLY:
                Add_group_apply(buf);
                break;
            case GROUP_APPLY:
                Group_apply(buf);
                break;
            case ADD_GROUP_ACCEPT_APPLY:
                Add_group_accept_apply(buf);
                break;
            //一起实现
            case DEL_GROUP_APPLY:
                Del_group_apply(buf);
                break;
            case WITHDRAW_GROUP_APPLY:
                Withdraw_group_apply(buf);
                break;
            case VIEW_GROUP_MEMBER_APPLY:
                View_group_member_apply(buf);
                break;
            case VIEW_ADD_GROUP_APPLY:
                View_add_group_apply(buf);
                break;
            case GROUP_CHAT_APPLY:
                Group_chat_apply(buf);
                break;
            case VIEW_GROUP_RECORD_APPLY:
                View_group_record_apply(buf);
                break;
            case SET_GROUP_ADMIN_APPLY:
                Set_group_admin_apply(buf);
                break;
            case KICK_APPLY:
                Kick_apply(buf);
                break;
                //
            //case SEND_FILE:
              //  {
                    //printf(">>>>>>\n");
                //    Recv_file(buf);
                    //printf("??????\n");
                  //  break;
                //}
            case RECV_FILE:
                Recv_file(buf);
                break;
            case RECV_APPLY:
                Recv_apply(buf);
                break;
        }
        //pthread_mutex_unlock(&mutex);
    }
    memset(buf,0,sizeof(buf));
}
void Turn_worker_thread()
{
    pthread_t pid_recv;
    pthread_create(&pid_recv,NULL,Recv_pack,NULL);
}
void Friend_box()
{
    //printf("111");
    Relation_t relation;
    //box_list_t pos=head;
    box_list_t pos;
    List_ForEach(head,pos)
    {
        if(pos->data.recver==username)
        {
            printf("uid\tsender\trecver\tmessage\n");
            printf("%d\t%d\t%d\t%s\n",username,pos->data.sender,pos->data.recver,pos->data.message);
        //pos=pos->next;
            break;
        }
    }
    List_ForEach(head,pos)
    {
        if(pos->data.recver==username)
        {
            List_DelNode(pos);
            char ch;
            printf("请输入同意还是不同意:\n");
            printf("同意[y or Y]不同意[n]");
        scanf("%c",&ch);
        if(ch=='Y' || ch=='y')
        {
                char str[BUFMAX];
                memset(str,0,sizeof(str));
                
                relation.flag=ADD_FRIEND_ACCEPT;
                strcpy(relation.message,"y");
                relation.sender=pos->data.recver;//被申请者
                relation.recver=pos->data.sender;//申请者
                //mes.send_fd=box_tt.send_fd;//申请者客户端端口号
                //mes.recv_fd=box_tt.recv_fd;//被申请者客户端端口号
                memcpy(str,&relation,sizeof(relation));
                if(send(cfd,str,sizeof(str),0)<0)
                {
                    my_err("send error",__LINE__);
                }
                printf("同意好友申请发送成功\n");
        }
        else
        {
            char str[BUFMAX];
            memset(str,0,sizeof(str));
                
            relation.flag=ADD_FRIEND_ACCEPT;
            strcpy(relation.message,"n");
            relation.sender=pos->data.recver;//被申请者
            relation.recver=pos->data.sender;//申请者
            //mes.send_fd=box_tt.send_fd;//申请者客户端端口号
            //mes.recv_fd=box_tt.recv_fd;//被申请者客户端端口号
            memcpy(str,&relation,sizeof(relation));
            if(send(cfd,str,sizeof(str),0)<0)
            {
                my_err("send error",__LINE__);
            }
            printf("拒绝好友申请发送成功\n");
        }
        }
    }
    /*
    system("clear");
    box_list_t pos;
    while(pos!=NULL)
    {
        printf("好友消息盒子信息\n");
        printf("uid\tsend_fd\trecv_fd\t消息\n");
        printf("%d\t%d\t%d\t%s\t",username,pos->data.send_fd,pos->data.recv_fd,pos->data.message);
        pos=pos->next;
    }
    apply_messgae mes;
    char str[BUFMAX];
    memset(&mes,0,sizeof(mes));
    mes.flag=ADD_FRIEND_ACCPET;
    //strcpy(mes.message,"y");
    mes.sender=username;//被申请者
    mes.recver=pos->data.sender;//申请者
    mes.send_fd=pos->data.send_fd;//申请者客户端端口号
    mes.recv_fd=pos->data.recv_fd;//被申请者客户端端口号
    memcpy(str,&mes,sizeof(mes));
    printf("cfd:%d\n",cfd);
    int id;
    List_ForEach(head,pos)
    {
        apply_messgae mes;
        memcpy(&mes,buf,sizeof(mes));
        printf("[%d]申请向您添加为好友\n",mes.recver);
        char ch;
        scanf("%c",&ch);
        getchar();
        if(ch=='Y' || ch=='y')
        {
            //strcpy(mes.message,"y");
            if(send(,str,sizeof(str),0)<0)
            {
                my_err("send error",__LINE__);
            }
            printf("同意好友申请发送成功\n");
        }
        else
        {
            char str1[BUFMAX];
            //memset(&mes,0,sizeof(mes));
            mes.flag=ADD_FRIEND_ACCPET;
            strcpy(mes.message,"n");
            mes.sender=username;//被申请者
            mes.recver=pos->data.sender;//申请者
            memcpy(str1,&mes,sizeof(mes));
            if(send(cfd,str1,sizeof(str1),0)<0)
            {
                my_err("send error",__LINE__);
            }
            printf("拒绝好友申请发送成功\n");
        }
     
    }   
*/
    
}

void Print_apply(char* buf)
{
    
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    //pthread_mutex_lock(&mutex);
   
    printf("%s\n",mes.messsge);
    //pthread_cond_wait(&cond,&mutex);
    //pthread_mutex_unlock(&mutex);    
}
void Add_friend_accept_appy(char* buf)
{
    apply_messgae mes;
    memcpy(&mes,buf,sizeof(mes));
    printf("%s\n",mes.message);
}
void Add_friend_apply(char* buf)
{
    //apply_messgae mes;
    box_t box;
    memcpy(&box,buf,sizeof(box));
    

    
    box_list_t new;
    new=(box_list_t)malloc(sizeof(box_node_t));
    printf("你有请求消息来了,在消息盒子\n");
   

    new->data.flag=box.flag;
    new->data.sender=box.sender;//申请者
    new->data.recver=box.recver;//被申请者
    new->data.send_fd=box.send_fd;//申请者客户端端口号
    new->data.recv_fd=box.recv_fd;//被申请者客户端端口号
    strcpy(new->data.message,box.message);
    List_AddTail(head,new);

   // printf("client message:%s\n",new->data.message);

    //printf("server send message:%s\n",box.message);
    //printf("server send flag:%d\n",box.flag);
    //printf("server/ friend send id:%d\n",box.sender);
    //printf("server/ friend recv id:%d\n",box.recver);
    //朋友客户端号
    //printf("server/ friend recv_fd:%d\n",new->data.recv_fd);
    //printf("server/ friend send_fd:%d\n",new->data.send_fd);
    


    usleep(100);

    /*if(mes.sender==username)
    {
        printf("不能向自己发送好友申请\n");
        return;
    }*/
 
}
void Add_friend()
{
    int id;
    puts("请输入想要添加好友的账号[id]:\n");
    scanf("%d",&id);
    if(id==username)
    {
        printf("请不要添加自己为好友\n");
        return ;
    }
    
    char buf[BUFMAX];
    sprintf(buf,"%d",id);
    printf("buf:%s\n",buf);

    char str[BUFMAX];
    memset(str,0,sizeof(str));
    Relation_t relation;
    relation.flag=ADD_FRIEND;
    relation.sender=username;//发送者
    relation.recver=id;//被申请者
    strcpy(relation.message,"");
    memcpy(str,&relation,sizeof(relation));

    //printf("client flag:%d\n",relation.flag);
    //printf("client message:%s\n",relation.message);
    //printf("client send:%d\n",relation.sender);
    //printf("client recv:%d\n",relation.recver);

    if(send(cfd,str,sizeof(str),0)<0)
    {
        my_err("send error!",__LINE__);
    }
}


void Del_friend()
{
    
    int id;
    puts("请输入要删除的好友账号:");
    scanf("%d",&id);

    char buf[BUFMAX];
    sprintf(buf,"%d",id);
    printf("buf:%s\n",buf);

    char str[BUFMAX];
    memset(str,0,sizeof(str));
    Relation_t relation;
    relation.flag=DEL_FRIEND;
    relation.sender=username;//发送者
    relation.recver=id;//被申请者
    strcpy(relation.message,"");
    memcpy(str,&relation,sizeof(relation));

    //printf("client flag:%d\n",relation.flag);
   // printf("client message:%s\n",relation.message);
   // printf("client send:%d\n",relation.sender);
   // printf("client recv:%d\n",relation.recver);

    if(send(cfd,str,sizeof(str),0)<0)
    {
        my_err("send error!",__LINE__);
    }

    return;
}
void Del_friend_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("%s\n",mes.messsge);

}
void Shield_friend()
{
    int id;
    printf("请输入你要屏蔽的好友账号:\n");
    scanf("%d",&id);
    char buf[BUFMAX];
    sprintf(buf,"%d",id);
    //printf("buf:%s\n",buf);

    char str[BUFMAX];
    memset(str,0,sizeof(str));
    Relation_t relation;
    relation.flag=SHIELD;
    relation.sender=username;//发送者
    relation.recver=id;//被申请者
    strcpy(relation.message,"");
    memcpy(str,&relation,sizeof(relation));

    if(send(cfd,str,sizeof(str),0)<0)
    {
        my_err("send error",__LINE__);
    }

    /*printf("client flag:%d\n",relation.flag);
    printf("client message:%s\n",relation.message);
    printf("client send:%d\n",relation.sender);
    printf("client recv:%d\n",relation.recver);*/


}
void Shield_friend_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("%s\n",mes.messsge);
}
void Unshield_friend()
{
    
    int id;
    printf("请输入你要解除屏蔽的好友账号:\n");
    scanf("%d",&id);
    char buf[BUFMAX];
    sprintf(buf,"%d",id);
    //printf("buf:%s\n",buf);

    char str[BUFMAX];
    memset(str,0,sizeof(str));
    Relation_t relation;
    relation.flag=UNSHIELD;
    relation.sender=username;//发送者
    relation.recver=id;//被申请者
    strcpy(relation.message,"");
    memcpy(str,&relation,sizeof(relation));

    if(send(cfd,str,sizeof(str),0)<0)
    {
        my_err("send error",__LINE__);
    }

   /* printf("client flag:%d\n",relation.flag);
    printf("client message:%s\n",relation.message);
    printf("client send:%d\n",relation.sender);
    printf("client recv:%d\n",relation.recver); */
}
void Unshield_friend_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("%s\n",mes.messsge);
}

//一起实现
//void Show_friend_status();
void View_friend_list()
{
    
    pthread_mutex_lock(&mutex_show);
    printf("-----------------------\n");
    printf("账号\t姓名\t状态\t关系\n");
    

    char str[BUFMAX];
    memset(str,0,sizeof(str));
    Friend_t friend;
    friend.flag=VIEW_FRIEND_LIST;
    friend.send=username;//发送者
    friend.recv=username;//接收者
    strcpy(friend.message,"");
    memcpy(str,&friend,sizeof(friend));

    if(send(cfd,str,sizeof(str),0)<0)
    {
        my_err("send error",__LINE__);
    }
    printf("-----------------------------\n");
    //printf("client flag:%d\n",friend.flag);
    //printf("client message:%s\n",friend.message);
    //printf("client send:%d\n",friend.send);
    //printf("client recv:%d\n",friend.recv); 
    pthread_cond_wait(&cond_show,&mutex_show);
     printf("=========================\n");
    pthread_mutex_unlock(&mutex_show);    
}
void View_friend_list_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    //printf("%s\n",mes.messsge);
    if(strcmp(mes.messsge,"over"))
    {
        printf("%s\n",mes.messsge);
    }
    else
    {
        pthread_mutex_lock(&mutex_show);
        pthread_cond_signal(&cond_show);
        pthread_mutex_unlock(&mutex_show);
    }
   //getch();
   //Friend_menu();
   //usleep(1000);
    return;
}


void Private_chat()
{
    int id;
    printf("请输入要私聊的好友账号:\n");
    scanf("%d",&id);
    //Chat_message mes;
    //memset(&mes, 0, sizeof(message));
    //char time[100];
    //char str[BUFMAX];

    /*char time[30];

    memset(time,0,sizeof(time));
    strcpy(time,Time());*/
    system("clear");

    printf("-----正在与 %d 聊天-----\n",id);
    printf("   --- quit 退出 ---\n");
   // getchar();
    //printf("=============================\n");
    //printf("time:%s",time);
    //scanf("%s",buf);
    //fgets(buf,sizeof(buf),stdin);
    while(1)
    {

        char str[BUFMAX];
       
        Chat_message mes;
        mes.flag=PRIVATE_CHAT;
        mes.sender=username;
        mes.recver=id;
        //strcpy(mes.time,time);
        //printf("client send:%s\n",time);
        char buf[256];
        memset(buf,0,sizeof(buf));
         fgets(buf,sizeof(buf),stdin);
        if(strcmp(buf,"quit\n")==0)
            break;

        strcpy(mes.message,buf);

        //保存光标位置
         //printf("\033[s");
        //fflush(stdout);
      

        //printf("\33[u");
        //fflush(stdout);
        /*printf("\33[s");
        fflush(stdout);

        //清除从光标到行尾的内容
        printf("\33[K\n\33[K\n\33[K\n");
        fflush(stdout);
        //恢复光标位置
        printf("\33[u");
        fflush(stdout);*/

        memcpy(str,&mes,sizeof(mes));
        if(send(cfd,str,sizeof(str),0)<0)
        {
            my_err("send error!",__LINE__);
        }
        //memset(buf,0,sizeof(buf));
        //memset(str,0,sizeof(str));
        //memset(&mes, 0, sizeof(message));


        //fgets(buf,sizeof(buf),stdin);
        //scanf("%s",buf);
    }
   /* printf("按任意键返回\n");
    getchar();*/
    return ;
}
char *Time()
{
    /*time_t time_t;
    char timep[100];
    int len;
    //时间
    time(&time_t);
    strcpy(timep,ctime(&time_t));
    len=strlen(timep);
    timep[len-5]='\0'; */

    char* str;
    time_t  time_t;
    time(&time_t);

    str=ctime(&time_t);
    str[strlen(str-1)]='\0';
    return str;
}
void Private_chat_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

        //printf("                       %s",mes.time);
       printf("账号\t姓名\t消息\n");

        printf("%s\n",mes.messsge); 
}


void View_chat_history()
{
    pthread_mutex_lock(&mutex_msg);
    int id;
    //system("clear");
    printf("请输入要查询聊天的好友账号:");
    scanf("%d",&id);
    
    char buf[BUFMAX];
    Chat_message mes;
    mes.flag=VIEW_CHAT_HISTORY;
    strcpy(mes.message,"");
    mes.sender=username;
    mes.recver=id;
    memcpy(buf,&mes,sizeof(mes));

    printf("\n");
    printf("--------------------聊天记录-------------------------\n");

    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error!",__LINE__);
    }


    pthread_cond_wait(&cond_msg,&mutex_msg);
    pthread_mutex_unlock(&mutex_msg);
}
void View_chat_history_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    
    if(strcmp(mes.messsge,"over"))
    {
        printf("%s\n",mes.messsge);
    } 
    else
    {
        pthread_mutex_lock(&mutex_msg);
        pthread_cond_signal(&cond_msg);
        pthread_mutex_unlock(&mutex_msg);       
    }
   // getch();
    return;

}

void Create_group()
{
    char buf[BUFMAX];
    Group_t group; 
 

    printf("请输入群名:");
    scanf("%s",group.group_name);
    printf("此群名只是当前群的称谓,创建者id为该群唯一标识\n");
    group.group_owner=username;
    group.flag=CREAT_GROUP;
    memcpy(buf,&group,sizeof(group));
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error",__LINE__);
    }    
}
void Create_group_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("%s\n",mes.messsge);

}
void Add_group()
{
   int id;
   printf("请输入要加入的群号:\n");
   scanf("%d",&id);

    char buf[BUFMAX];
    Relation_t relation;
    relation.flag=ADD_GROUP;
    relation.sender=username;
    relation.recver=id;
    strcpy(relation.message,"");
    memcpy(buf,&relation,sizeof(relation));
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error",__LINE__);
    }    
}
void Add_group_apply(char* buf)
{
    box_t box;
    memcpy(&box,buf,sizeof(box));
    

    
    box_list_t new;
    new=(box_list_t)malloc(sizeof(box_node_t));
    printf("你有群请求消息来了,在消息盒子\n");
   

    new->data.flag=box.flag;
    new->data.sender=box.sender;//申请者
    new->data.recver=box.recver;//管理员
    new->data.send_fd=box.send_fd;//申请者客户端端口号
    new->data.recv_fd=box.recv_fd;//管理员客户端端口号
    strcpy(new->data.message,box.message);
    List_AddTail(fhead,new);

   /* printf("client message:%s\n",new->data.message);

    printf("server send message:%s\n",box.message);
    printf("server send flag:%d\n",box.flag);
    printf("server/ group apply send id:%d\n",box.sender);
    printf("server/ group apply recv id:%d\n",box.recver);
    //管理员客户端号
    printf("server/ group recv_fd:%d\n",new->data.recv_fd);
    printf("server/ group send_fd:%d\n",new->data.send_fd);*/
    


    usleep(100);
}
void Group_box()
{
    //printf("111");
    Relation_t relation;

    box_list_t pos;
    List_ForEach(fhead,pos)
    {
        if(pos->data.recver==username)
        {
            printf("uid\tsender\trecver\tmessage\n");
            printf("%d\t%d\t%d\t%s\n",username,pos->data.sender,pos->data.recver,pos->data.message);
        //pos=pos->next;
            break;
        }
    }
    List_ForEach(fhead,pos)
    {
        if(pos->data.recver==username)
        {
            List_DelNode(pos);
            char ch;
            printf("请输入同意还是不同意:\n");
             printf("同意[y or Y]不同意[n]");
            scanf("%c",&ch);
            if(ch=='Y' || ch=='y')
            {
                    char str[BUFMAX];
                    memset(str,0,sizeof(str));
                    
                    relation.flag=ADD_GROUP_ACCEPT;
                    strcpy(relation.message,"y");
                    relation.sender=pos->data.recver;//管理员
                    relation.recver=pos->data.sender;//申请者
                    //mes.send_fd=box_tt.send_fd;//申请者客户端端口号
                    //mes.recv_fd=box_tt.recv_fd;//被申请者客户端端口号
                    memcpy(str,&relation,sizeof(relation));
                    if(send(cfd,str,sizeof(str),0)<0)
                    {
                        my_err("send error",__LINE__);
                    }
                    printf("同意群申请发送成功\n");
            }
            else
            {
                char str[BUFMAX];
                memset(str,0,sizeof(str));
                    
                relation.flag=ADD_GROUP_ACCEPT;
                strcpy(relation.message,"n");
                relation.sender=pos->data.recver;//被申请者
                relation.recver=pos->data.sender;//申请者
                //mes.send_fd=box_tt.send_fd;//申请者客户端端口号
                //mes.recv_fd=box_tt.recv_fd;//被申请者客户端端口号
                memcpy(str,&relation,sizeof(relation));
                if(send(cfd,str,sizeof(str),0)<0)
                {
                    my_err("send error",__LINE__);
                }
                printf("拒绝群申请发送成功\n");
            }
        }
    }
}
void Add_group_accept_apply(char* buf)
{
    apply_messgae mes;
    memcpy(&mes,buf,sizeof(mes));
    printf("%s\n",mes.message);
}
void Group_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("%s\n",mes.messsge);
}

//一起实现
//void Del_group();
void Withdraw_group()
{
    int id;
    printf("请输入你要退出的群号:");
    scanf("%d",&id);

    char buf[BUFMAX];
    Relation_t relation;
    relation.flag=WITHDRAW_GROUP;
    relation.sender=username;
    relation.recver=id;
    strcpy(relation.message,"");
    memcpy(buf,&relation,sizeof(relation));
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error",__LINE__);
    }    
    
    

}
void Withdraw_group_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("%s\n",mes.messsge);
}
void Del_group_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("%s\n",mes.messsge);
}

void View_group_member()
{
    pthread_mutex_lock(&mutex_gmb);
    int id;
    printf("请输入你要查询的群号:");
    scanf("%d",&id);
    
    char buf[BUFMAX];
    Relation_t relation;
    relation.flag=VIEW_GROUP_MEMBER;
    relation.sender=username;
    relation.recver=id;
    strcpy(relation.message,"");
    memcpy(buf,&relation,sizeof(relation));

    printf("----------------------------\n");
    printf("账号\t姓名\t状态\n");
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error",__LINE__);
    }    

    printf("-----------------------------\n");
    pthread_cond_wait(&cond_gmb,&mutex_gmb);
    printf("=========================\n");
    pthread_mutex_unlock(&mutex_gmb);
 
}
void View_group_member_apply(char* buf)
{
    
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    if(strcmp(mes.messsge,"over"))
    {
        printf("%s\n",mes.messsge);
    }
    else
    {
        pthread_mutex_lock(&mutex_gmb);
        pthread_cond_signal(&cond_gmb);
        pthread_mutex_unlock(&mutex_gmb);
    }
    //getch();
    return;
}
void View_add_group()
{
    pthread_mutex_lock(&mutex_add_grp);

    char str[BUFMAX];
    memset(str,0,sizeof(str));
    Relation_t relation;
    relation.flag=VIEW_ADD_GROUP;
    relation.sender=username;//发送者
    relation.recver=username;//群号
    strcpy(relation.message,"");
    memcpy(str,&relation,sizeof(relation));

    printf("----------------------------\n");
    printf("群号\t群名\t地位\n");
    if(send(cfd,str,sizeof(str),0)<0)
    {
        my_err("send error",__LINE__);
    }
    printf("-----------------------------\n");
     //阻塞等待
    pthread_cond_wait(&cond_add_grp,&mutex_add_grp);
    printf("=============================\n");
    pthread_mutex_unlock(&mutex_add_grp);
 
}
void View_add_group_apply(char* buf)
{
    
    message mes;
    memcpy(&mes,buf,sizeof(mes));

    if(strcmp(mes.messsge,"over"))
    {
        printf("%s\n",mes.messsge);
    }
    else
    {
        //唤醒阻塞状态的好友列表
        pthread_mutex_lock(&mutex_add_grp);
        pthread_cond_signal(&cond_add_grp);
        pthread_mutex_unlock(&mutex_add_grp);
    }
   // getch();
	return;

}


void Group_chat()
{
    int id;
    printf("请输入要发送聊天的群账号:\n");
    scanf("%d",&id);
    while(getchar()!='\n');
    //Chat_message mes;
    //memset(&mes, 0, sizeof(message));
  
 
    //char buf[256];
    //char time[100];
    //char str[BUFMAX];

    /*char time[30];

    memset(time,0,sizeof(time));
    strcpy(time,Time());*/
    system("clear");

    printf("-----正在与 %d 群里聊天-----\n",id);
    printf("   --- quit 退出 ---\n");

    printf("=============================\n");
    //printf("time:%s",time);
    //fgets(buf,sizeof(buf),stdin);
    //scanf("%s",buf);
    while(1)
    {

        char str[BUFMAX];
       
        Chat_message mes;
        mes.flag=GROUP_CHAT;
        mes.sender=username;
        mes.recver=id;
        //strcpy(mes.time,time);
        //printf("client send:%s\n",time);

        char buf[256];
        memset(buf,0,sizeof(buf));
         fgets(buf,sizeof(buf),stdin);
        if(strcmp(buf,"quit\n")==0)
            break;

        strcpy(mes.message,buf);
        memcpy(str,&mes,sizeof(mes));
        if(send(cfd,str,sizeof(str),0)<0)
        {
            my_err("send error!",__LINE__);
        }
        //memset(buf,0,sizeof(buf));
        //memset(str,0,sizeof(str));
        //memset(&mes, 0, sizeof(message));
        //scanf("%s",buf);

        //fgets(buf,sizeof(buf),stdin);

    }

}
void Group_chat_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    /*if(strcmp(mes.message,"d")==0)
        printf("[%d]不在线 \n",mes.recver);*/
    //else
    //{
        //printf("                       %s",mes.time);
    
  
            printf("账号\t名字\t信息\n");
            printf("%s\n",mes.messsge);
   
}
void View_group_record()
{
    pthread_mutex_lock(&mutex_gcord);
    printf("--------------------聊天记录-------------------------\n");

    int id;
    //system("clear");
    printf("请输入要查询聊天的群账号:");
    scanf("%d",&id);
    
    char buf[BUFMAX];
    Chat_message mes;
    mes.flag=VIEW_GROUP_RECORD;
    strcpy(mes.message,"");
    mes.sender=username;
    mes.recver=id;
    memcpy(buf,&mes,sizeof(mes));
    if(send(cfd,buf,sizeof(buf),0)<0)
    {
        my_err("send error!",__LINE__);
    }
    

    pthread_cond_wait(&cond_gcord,&mutex_gcord);
     printf("=========================\n");
    pthread_mutex_unlock(&mutex_gcord);  
}
void View_group_record_apply(char* buf)
{
  
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    if(strcmp(mes.messsge,"over"))
    {
         printf("%s\n",mes.messsge);
    }
    else
    {
        pthread_mutex_lock(&mutex_gcord);
        pthread_cond_signal(&cond_gcord);
        pthread_mutex_unlock(&mutex_gcord);     
           
    }
    //  getch();
    return;
}

void Set_group_admin()
{
    char buf[BUFMAX];
    Group_leader leader;
    leader.flag=SET_GROUP_ADMIN;
    leader.sender=username;
    printf("请输入你设置的群号:");
    scanf("%d",&leader.recver);
    printf("请输入你要设置管理员的账号：");
    scanf("%d",&leader.admin);
    strcpy(leader.message,"");

    memcpy(buf,&leader,sizeof(leader));
    if(send(cfd,buf,sizeof(buf),0)<0) 
    {
        my_err("send error",__LINE__);
    }
}
void Set_group_admin_apply(char *buf)
{
    
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    printf("%s\n",mes.messsge);
}
void Kick()
{
    char buf[BUFMAX];
    Group_leader leader;
    leader.flag=KICK;
    leader.sender=username;
    printf("请输入你设置的群号:");
    scanf("%d",&leader.recver);
    printf("请输入你要删除成员的账号：");
    scanf("%d",&leader.admin);
    strcpy(leader.message,"");

    memcpy(buf,&leader,sizeof(leader));
    if(send(cfd,buf,sizeof(buf),0)<0) 
    {
        my_err("send error",__LINE__);
    }
    
}
void Kick_apply(char *buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    printf("%s\n",mes.messsge);
}
void File_menu()
{
    int choice=1;
    system("clear");
    
    while(choice)
    {
        usleep(1000);
        printf("\t\t\033[;33m\033[1m*********文件管理**********\033[0m\n");
        printf("\t\t\033[1;33m|\033[0m-1.查看本地客户端目录文件-\033[1;33m|\033[0m\n");
        printf("\t\t\033[1;33m|\033[0m----------2.上传----------\033[1;33m|\033[0m\n");
        printf("\t\t\033[1;33m|\033[0m----------3.下载----------\033[1;33m|\033[0m\n");
        printf("\t\t\033[1;33m|\033[0m----------0.退出----------\033[1;33m|\033[0m\n");

        printf("\t\tchoice:\n");
        //printf("请输入选择:");
        scanf("%d",&choice);
        while(getchar() != '\n');
        switch (choice)
        {
            case 1:
                Client_list();
                break;
            case 2:
               //Upload();
                Send_file();
                break;
            case 3:
                Download();
                break;
            case 0:
                break;
        }
    }
    return;
}
void Client_list()
{
    printf("当前客户端目录列表:\n");
	DIR *dir;
	
	struct dirent *dirent;
    char name[256];
    int i,j;
    int count=0;


    char filenames[256][PATH_MAX+1],temp[PATH_MAX+1];
    //获取该目录下文件总数和最长的文件名
    dir=opendir("."); //统计文件名数量
    if(dir==NULL)
        my_err("opendir",__LINE__);
    while((dirent=readdir(dir))!=NULL)
    {
        if(g_maxlen<strlen(dirent->d_name))
                g_maxlen=strlen(dirent->d_name);
        count++;
    }

        if(count>256)
            my_err("too mant files under this dir",__LINE__);
        
        closedir(dir);
    

    int len=strlen(".");
        //获取该目录下所有的文件名
        dir=opendir(".");
        for(i=0;i<count;i++)
        {
            dirent=readdir(dir);
            if(dirent==NULL)
                my_err("readdir",__LINE__);

            strncpy(filenames[i],"",len);
            filenames[i][len]='\0';
            strcat(filenames[i],dirent->d_name);
            filenames[i][len+strlen(dirent->d_name)]='\0';
        }
        closedir(dir);

        //使用冒泡法对文件名进行排序，排序后文件名按字母顺序存储于filenames
        for(i=0;i<count-1;i++)
        {
            for(j=0;j<count-1-i;j++)
            {
                if(strcmp(filenames[j],filenames[j+1])>0)
                {
                    strcpy(temp,filenames[j+1]);
                    temp[strlen(filenames[j+1])]='\0';
                    strcpy(filenames[j+1],filenames[j]);
                    filenames[j+1][strlen(filenames[j])]='\0';
                    strcpy(filenames[j],temp);
                    filenames[j][strlen(temp)]='\0';
                }
            }
        }
    for(i=0;i<count;i++)
        Display(filenames[i]);

	/*while((dirent=readdir(dir))!=NULL)
	{
		printf("%s ",dirent->d_name);
        printf("\n");

    }*/

      

	printf("\n");

    printf("\033[;33m请按任意键继续\033[0m\n");
	getch();
	return;
}
void Display(char* pathname)
{
    int i,j;
    struct stat buf;
    char name[256];

    //从路径中解析出文件名
    for(i=0,j=0;i<strlen(pathname);i++)
    {
        if(pathname[i]=='/')
        {
            j=0;
            continue;
        }
        name[j++]=pathname[i];
    }
    name[j]='\0';
    

    //如果本行不足以打印一个文件名则换行
    if(g_leave_len<g_maxlen)
    {
        printf("\n");
        g_leave_len=MAXLEN;
    }

    len=strlen(name);
    len=g_maxlen-len;
   
    printf("\033[;33m%s\033[0m",name);

    for(i=0;i<len;i++)
        printf(" ");

    printf(" ");

    //下面的2指示空两格
    g_leave_len-=(g_maxlen+2);//一行中剩下的字符数量

    //Display_single(name);
}

/*void Display_single(char* name)
{
    int i,len;
     //如果本行不足以打印一个文件名则换行
    if(g_leave_len<g_maxlen)
    {
        printf("\n");
        g_leave_len=MAXLEN;
    }

    len=strlen(name);
    len=g_maxlen-len;
   
    printf("\033[;33m%s\033[0m",name);

    for(i=0;i<len;i++)
        printf(" ");

    printf(" ");

    //下面的2指示空两格
    g_leave_len-=(g_maxlen+2);//一行中剩下的字符数量
}*/

/*void Send_file()
{
    int id;
    printf("请输入要发送文件的好友账号:\n");
    scanf("%d",&id);

  
	printf("请输入文件名:\n");
	char pathname[100];


	//get_str(pathname,100);
    scanf("%s",pathname);
    
    int fd=open(pathname,O_RDONLY);
    if(fd<0)
    {
       printf("请输入正确的文件路径\n");
       return ;
    }

    file_t file;
    memset(&file,0,sizeof(file));
    file.recver=id;
    file.sender=username;
    file.flag=SEND_FILE;
    strcpy(file.file_name,pathname);



    struct stat stat;
	int fs=fstat(fd,&stat);
	long file_size=0;
	file_size=stat.st_size;
   // file.file_size=file_size;
    printf("file.file_size=%d\n",file.file_size);
    
    printf("sizeof(file)=%ld\n",sizeof(file));
    printf("file_size=%ld\n",file_size);
    
    int filefd = open(pathname,O_RDONLY);
    assert(filefd>0);
    struct stat file_stat;
    //为了获取文件大小
    fstat(filefd,&file_stat);

    sendfile(connfd,filefd,NULL,file_stat.st_size);
    close(filefd);
 
    close(fd);    
}*/
void Send_file()
{
    int id;
    printf("请输入要发送文件的好友账号:\n");
    scanf("%d",&id);

  
	printf("请输入文件名:\n");
	char pathname[100];


	//get_str(pathname,100);
    scanf("%s",pathname);
    
    /*int fd=open(pathname,O_RDONLY);
    if(fd<0)
    {
       printf("请输入正确的文件路径\n");
       return ;
    }


    struct stat st;
    long int size=stat(pathname, &st);
    if(size < 0)
    {
        printf("file stat error\n");
        exit(1);
    }
    printf("file size:%ld\n",st.st_size);

    off_t pos=lseek(fd,0,SEEK_SET);
    if(pos < 0)
    {
        printf("get file_pos error\n");
        exit(1);
    }*/

    
    file_t file;
    file.flag=UPLOAD;
    strcpy(file.file_name,pathname);
    file.recver=id;
    file.sender=username;

    char buf[BUFMAX];
    memcpy(buf,&file,sizeof(file));
    send(cfd,buf,sizeof(buf),0);
    

    char buff[1024];
    memset(buff,0,sizeof(buff));
    // FILE *fp = fopen(file.file_name,"r");
    int fp=open(pathname,O_RDONLY);
    if(fp<0)
    {
        perror("open failed");
        return;
    }
    else 
    {
        int i = 0;

        memset(buff,0,sizeof(buff));
        int len=0;
        //while((len=fread(buff,sizeof(char),1024,fp))> 0)
        while((len=read(fp,buff,sizeof(buff)))>0)
        {
                            
            i++;
                                
            //  printf("len= %d\n",len);
            if(send(cfd,buff,len,0)<=0)
            {
                printf("Send file failed\n");
                break;
            }
            memset(buff,0,sizeof(buff));

        }
        close(fp);
        memset(buff,0,sizeof(buff));
                            
        sleep(1);
        strcpy(buff,"end");
        send(cfd,buff,strlen(buff),0);
    }           


    /*long int ssize = 0;
    char buffer[BUFMAX];
    long int n=0;
    
    while(1)
    {
        bzero(buffer,1024);
        int rn = read(fd, buffer, 1024);
        int wn = write(cfd, buffer, 1024);
        if(n >= st.st_size)
        {
            printf("size:%ld\n", n);
            printf("size:%ld\n", st.st_size);
            break;
        }
        n += rn;
    }
    if(n==-1)
    {
        printf("send file error\n");
        exit(1);
    }*/

}

void Download()
{
    file_t file;

    //char file_name[100];
    printf("请输入下载文件名:\n");
    scanf("%s",file.file_name);

    file.flag=DOWNLOAD;
    file.sender=username;
    file.recver=0;//服务器接收

    char buf[BUFMAX];
    memcpy(buf,&file,sizeof(file));

    send(cfd,buf,sizeof(buf),0);

}

void Recv_file(char* buf)
{

    file_t file;
    memcpy(&file,buf,sizeof(file));

    char buff[1024];
    memset(buff,0,sizeof(buff));
    char file_name[100];
    memset(file_name,0,sizeof(file_name));


         
    strcpy(file_name,file.file_name);    
    printf("file name=%s\n",file_name);
    int fp = open(file_name,O_CREAT  | O_TRUNC ,0666);
    if(fp < 0)
    {
        perror("open file fail");
    }
    close(fp);

    fp = open(file_name,O_APPEND | O_WRONLY);
    if(fp<0)
        perror("open file fail");

    memset(buff,0,sizeof(buff));
    int len=0;
    int write_len;
    int i = 0;
    while( (len=recv(cfd,buff,sizeof(buff),0) ) > 0 )
    {
                        
                    
        if(strcmp(buff,"end") == 0)
            break;
                    
        if(len<0)
        {
            printf("Download file error\n");
            break;
        }
                    
        write_len=write(fp,buff,len);
        /*                
        if(write_len!= len)
        {
            printf("write failed\n");
            return 0;
        }*/
        memset(buff,0,sizeof(buff));
                    
    }
    close(fp);
   // arg=(char*)arg;
    /*int fd;
    file_t file;
    memcpy(&file,buf,sizeof(file));
    printf("file name:%s\n",file.file_name);
    //如果文件不存在创建文件
    if((fd=open(file.file_name,O_RDWR | O_CREAT | O_APPEND,0777))<0)
    {
        my_err("open file_name error",__LINE__);
    }
    
    
    int ret=write(fd,file.data,file.file_size);
    if(ret<0)
    {
        my_err("write file error",__LINE__);
    }*/


    /*
    printf("ret=%d\n",ret);
    printf("file_name=%s\n",file.file_name);
    printf("recver=%d\n",file.recver);
    printf("sender=%d\n",file.sender);
    printf("file_size=%d\n",file.file_size);
    */

//    close(fd);
}

void Recv_apply(char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    printf("%s\n",mes.messsge);

}
void Friend_menu()
{
    int choice=1;
    system("clear");
    
    while(choice)
    {
        usleep(1000);
        printf("\t\t\033[;36m\033[1m*********朋友管理*********\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m--------1.添加好友-------\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m--------2.好友消息盒子-------\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m--------3.删除好友-------\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m--------4.私聊好友-------\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m--------5.屏蔽好友-------\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m------6.解除屏蔽好友-----\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m------7.查看好友列表-----\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m------8.查看聊天记录-----\033[1;36m|\033[0m\n");
        printf("\t\t\033[1;36m|\033[0m------0.退出-----\033[1;36m|\033[0m\n");
        printf("\t\tchoice:\n");
        //printf("请输入选择:");
        scanf("%d",&choice);
        while(getchar() != '\n');
        switch (choice)
        {
            case 1:
                Add_friend();
                break;
            case 2:
                Friend_box();
                break;
            case 3:
                Del_friend();
                break;
            case 4:
                Private_chat();
                break;
            case 5:
                Shield_friend();
                break;
            case 6:
                Unshield_friend();
                break;
            case 7:
                View_friend_list();
                break;
            case 8:
                View_chat_history();
                break;
            case 0:
                break;
        }
    }
    return;
}
void Group_menu()
{
    system("clear");
    int choice=1;
    while(choice)
    {
        usleep(1000);
        //printf("\t\t\033[;34m\033[1m*********群管理*********\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------1.创建群-------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------2.添加群-------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------3.群消息盒子-------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------4.退群---------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------5.群聊---------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------6.已加群-------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------7.群成员-------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m-----8.查看聊天记录----\033[1;34m|\033[0m\n");
        //printf("\t\t\033[1;34m|\033[0m------群管理权限-----\033[1;34m|\033[0m\n");
        //printf("\t\t\033[1;34m|\033[0m-------------------\033[1;34m|\033[0m\n");
        //printf("\t\t\033[1;34m|\033[0m*群主--1,2,3/管理员--3权限*\033[1;34m|\033[0m\n");
        //printf("\t\t\033[1;34m|\033[0m-------------------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m------9.设置管理员------\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m------10.踢人-----\033[1;34m|\033[0m\n");
        printf("\t\t\033[1;34m|\033[0m--------0.退出--------\033[1;34m|\033[0m\n");
        printf("\t\tchoice:\n");
        scanf("%d",&choice);
        while(getchar() != '\n');
        switch (choice)
        {
            case 1:
                Create_group();
                break;
            case 2:
                Add_group();
                break;
            case 3:
                Group_box();
                break;
            case 4:
                Withdraw_group();
                break;
            case 5:
                Group_chat();
                break;
            case 6:
                View_add_group();
                break;
            case 7:
                View_group_member();
                break;
            case 8:
                View_group_record();
                break;
            case 9:
                Set_group_admin();
                break;
            case 10:
                Kick();
                break;
            case 0:
                break;
        }
    }
    return;
}

void Print_menu()
{
    printf("\t\t\033[1;34m**************************\033[0m\n");
    printf("\t\t\033[1;34m|        1.好友管理      \033[1;34m|\033[0m \n");
    printf("\t\t\033[1;34m|        2.群管理        \033[1;34m|\033[0m \n");
    printf("\t\t\033[1;34m|        3.文件管理      \033[1;34m|\033[0m \n");
    printf("\t\t\033[1;34m|        0.退出          \033[1;34m|\033[0m \n");
    printf("\t\t\033[1;34m**************************\033[0m\n");
    printf("\t\tchoice:");
    printf("\n");
}
void Menu()
{
    system("clear");
    int choice=1;
    //char choice_t[50];

    do
    {
       
        Print_menu();
        scanf("%d",&choice);
        //get_choice_int(0,3);
         //while(getchar()!='\n');
        //choice=Get_choice(choice_t);
       
        switch (choice)
        {
            case 1:
                Friend_menu();
                break;
            case 2:
                Group_menu();
                break;
            case 3:
                File_menu();
                break;
            case 0:
                Exit();
                //Login_menu();
                break;
        }
    }while(choice!=0);
 
}
void my_err(const char* err_string,int line)
{
    fprintf(stderr,"line:%d",line);
    perror(err_string);
    exit(1);
}
void Clear_buffer()
{
    char ch;
    while(getchar()!='\n')
		continue;
    /*while((ch=getchar())!='\n' && ch!=EOF)
            continue;*/
}
void display(char* str)
{
    int i;
    system("clear");
    for(i=0;i<50;i++)
        putchar('-');
    putchar('\n');
    printf("       %s\n",str);
    for(i=0;i<50;i++)
        putchar('-');
    putchar('\n');
    return;
}
int get_choice_int(int min, int max)
{
	int input;
	char ch;

	while ( (scanf("%d", &input) != 1) || (input < min) || (input > max))
	{
		while ((ch =getchar()) != '\n')
            continue;
		printf("请输入正确选项：\n");
	}

    while ((ch =getchar()) != '\n')
            continue;

	return input;
}

/*char getch()
{
	char ch;

    system("stty -echo");//不回显
    system("stty -icanon");//设置一次性读完操作，如使用getchar()读操作,不需要按回车
    ch = getchar();
    system("stty icanon");//取消上面的设置
    system("stty echo");//回显

    return ch;
}*/
//　修改终端的控制方式，1取消回显、确认　２获取数据　3还原
int getch()
{
    // 记录终端的配置信息
    struct termios old;
    // 获取终端的配置信息
    tcgetattr(STDIN_FILENO,&old);
    // 设置新的终端配置   
    struct termios new1 = old;
    // 取消确认、回显
    new1.c_lflag &= ~(ICANON|ECHO);
    // 设置终端配置信息
    tcsetattr(STDIN_FILENO,TCSANOW,&new1);

    // 在新模式下获取数据   
    int key_val = 0; 
    do{
    	key_val += getchar();
    }while(stdin->_IO_read_end - stdin->_IO_read_ptr);

    // 还原配置信息
    tcsetattr(STDIN_FILENO,TCSANOW,&old); 
    return key_val; 
}

char* Get_string(char* buf,int len)
{
    char* str;
    int i=0;
    str=fgets(buf,len,stdin);
	if(str!=NULL)
	{
		while(str[i]!='\0' && str[i]!='\n')
			i++;
		if(str[i]=='\n')
			str[i]='\0';
		else
			while(getchar()!='\n')
				continue;
	}
	return str;
}
char* get_str(char* str,size_t len)
{
	if(NULL == str)
	{
		puts("空指针异常\n");
		return NULL;
	}

	char *in=fgets(str,len,stdin);
	
	size_t cnt = strlen(str);
	if('\n' == str[cnt-1])
	{
		str[cnt-1] = '\0';
	}
	
	clear_stdin();

	return str;
}
void clear_stdin()
{
	stdin->_IO_read_ptr = stdin->_IO_read_end;//清理输入缓冲区
}
int Get_choice(char *choice_t)
{
    int choice =0;
    for(int i=0;i<strlen(choice_t) ;i++)
        if(choice_t[i]<'0' || choice_t[i]>'9')
            return -1;
    for(int i=0;i<strlen(choice_t);i++)
    {
        int t=1;
        for(int j=1;j<strlen(choice_t)-i;j++)
        {
            t *=10;
        }
        choice += t*(int)(choice_t[i] - 48);
    }
    return choice;
}

