#include "server.h"
#define EPOLLEVENT 1024
#define BUFMAX 1024
#define BUF 2048


char *Server_time();
int	Set_no_block(int sfd);

int sys_log;
MYSQL mysql;

/*
void Add_node(int fd,int id,char* name);
void Send_register_pack(int fd,int flag,char* buf,int id);
int Get_status(int id);
char* Get_name(int id);*/
int main()
{

   


    /*if((sys_log=open("sys_log",O_WRONLY | O_CREAT | O_APPEND,S_IRUSR|S_IWUSR))<0)
    {
        sys_err("open error",__LINE__);
        return 0;
    }
    dup2(sys_log,1);*/

    
    signal(SIGINT,Signal_close);

    //pthread_mutex_init(&mutex,NULL);
    //pthread_cond_init(&cond,NULL);

    Connect_mysql();
    /*printf("线程池启动\n");
    pool_init(MAX_THREAD_NUM);
    printf("线程池启动成功!\n");
    sleep(2);*/
    pool_init(50);
    //sleep(2);
    //Read_from_mysql();
    Init_socket();


    //threadpool_destroy();

}
void Signal_close(int i)
{
    close(sys_log);
    Close_mysql(mysql);
    printf("服务器关闭\n");
    exit(1);
}


char *Server_time()
{
	time_t ctime;//服务器时间
	struct tm *server_time;
	time(&ctime);
	server_time=localtime(&ctime);
	return asctime(server_time);
}

int	Set_no_block(int sfd)
{
	/* 内层调用fcntl()的F_GETFL获取flag，
	 * 外层fcntl()将获取到的flag设置为O_NONBLOCK非阻塞*/
	if( fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) ) == -1)
	{	
        return -1;
    }
	return 0;
}

void Init_socket(int argc,char *argv[])
{
    struct stat stat_buf;
	const char* file_name=argv[1];   
    int file_fd=open(file_name,O_RDONLY);
	fstat(file_fd,&stat_buf);
	close(file_fd); 

    List_Init(list_ser,server_user_node_t);
    printf("服务端启动\n");
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    socklen_t cli_addr_len;

    lfd=Socket(AF_INET,SOCK_STREAM,0);


    Set_no_block(lfd);
    //端口复用
    int opt=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(opt));

    bzero(&serv_addr,sizeof(serv_addr));

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(SERV_PORT);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    Bind(lfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    Listen(lfd,128);

    printf("服务器启动成功!\n");

    epfd=epoll_create(EPOLLEVENT);
    struct epoll_event tep,ep[EPOLLEVENT];
    tep.events=EPOLLIN;
    tep.data.fd=lfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&tep);

    int i;
    int ret;
    pthread_t pid;
    char buf[BUFMAX];
    //memset(buf,0,sizeof(buf));
    while(1)
    {
        ret=epoll_wait(epfd,ep,EPOLLEVENT,-1);
        for(i=0;i<ret;i++)
        {

            //printf("the event is %x\n",ep[i].events);
            int fd=ep[i].data.fd;

            if (!(ep[i].events & EPOLLIN))      //如果不是"读"事件, 继续循环
                continue;

            if(ep[i].data.fd==lfd)
            {
                cli_addr_len=sizeof(cli_addr);
                cfd=Accept(lfd,(struct sockaddr*)&cli_addr,&cli_addr_len);
                printf("连接到新的客户端ip:%s\n端口号:%d\n",inet_ntoa(cli_addr.sin_addr),cli_addr.sin_port);
                
                /*设置非阻塞io*/
				if(Set_no_block(cfd) != 0 )
				{
					printf("SET_no_black\n");
					printf("%s",Server_time());
					close(cfd);
					continue;
				}
                
                tep.events=EPOLLIN;
                tep.data.fd=cfd;
                if(epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&tep)<0)
                {
                    printf("EPOLL_CTL add client error\n");
                    printf("%s",Server_time());
					
                    close(cfd);
					continue;
                }

            }
            /*else if(ep[i].events & EPOLLOUT) 
            {
                printf("start to sendfile !\n");
                printf("处理写事件\n");
                int write;
                write=send(fd,buf,strlen(buf),0);
                if(write == -1)
                {
                    my_err("write event error",__LINE__);
                    close(fd);
                }
                else
                {
                    printf("发送消息成功\n");
                }
                memset(buf,0,BUFMAX);
            }*/
            /*else if(ep[i].events & EPOLLOUT)
            {

				printf("start to sendfile !\n");
				int send_ret=0;
                int left=stat_buf.st_size;
				file_fd=open(file_name,O_RDONLY);

				while(left>0)
                {
					send_ret=sendfile(cfd,file_fd,NULL,BUF);
					if(send_ret<0 && errno==EAGAIN)
                    {
						continue;
					}
                    else if(send_ret==0)
                    {
						break;
					}
                    else
                    {
						left-=send_ret;
					}
				}

				printf("sendfile over !\n");
				close(file_fd);

				tep.data.fd=cfd;
				epoll_ctl(epfd,EPOLL_CTL_DEL,cfd,&tep);

				close(cfd);
            }*/
            else if(ep[i].events & EPOLLIN)
            {
                memset(buf,0,sizeof(buf));
                int n=recv(ep[i].data.fd,buf,sizeof(buf),MSG_WAITALL);
                int fd=ep[i].data.fd;
                printf("\n");
                /*printf("%d\n",recv_t.data.send_fd);
                printf("%d\n",recv_t.data.recv_fd);
                printf("server message%s\n",recv_t.data.message);
                printf("server recv:%s\n",recv_t.data.send_name);*/


                if(n<0)
                {
                    close(ep[i].data.fd);
                    sys_err("recv error!",__LINE__);
                    continue;
                }
                else if(n==0)
                {
                    printf("??????\n");
                    server_list_t pos;
                    List_ForEach(list_ser,pos)
                    {
                        if(pos->data.connfd==ep[i].data.fd)
                        {    
                            printf("sssssssssssss\n");
                           
                            char buf_t[BUFSIZ];
                            sprintf(buf_t,"update account set status=0 where id=%d",pos->data.id);
                            printf("buf_t:%s\n",buf_t);
                            int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
                            if(ret)
                            {
                                Mysql_with_error(&mysql);
                            }
                            //printf("用户[%d]已经下线\n",pos->data.id);
                            printf("账号[%d]下线了:%d\n",pos->data.id,pos->data.connfd);

                            List_FreeNode(pos);  
                            break;
                        }
                    }
                    tep.data.fd=ep[i].data.fd;
                    printf("客户端:%d连接断开\n",tep.data.fd);
                   
                    epoll_ctl(epfd,EPOLL_CTL_DEL,ep[i].data.fd,&tep);
                    close(ep[i].data.fd);
                    continue;
                }

                int flag;
                memcpy(&flag,buf,sizeof(int));
                printf("flag:%d\n",flag);
                
                switch(flag)
                {
                    case LOGIN:
                        Login(fd,buf);
                        break;
                    case REGISTER:
                        Register(fd,buf);
                        break;
                    case EXIT:
                        Exit(fd,buf);
                        break;
                    case ADD_FRIEND:
                        Add_friend(fd,buf);
                        break;
                    case ADD_FRIEND_ACCEPT:
                        Add_friend_accept(fd,buf);
                        break;
                    case DEL_FRIEND:
                        Del_friend(fd,buf);
                        break;
                    case PRIVATE_CHAT:
                        Private_chat(fd,buf);
                        break;
                    //写在一起比较好
                    //case SHOW_FRIEND_STATUS:
                    case VIEW_FRIEND_LIST:
                        View_friend_list(fd,buf);
                        break;
                    case SHIELD:
                        Shield_friend(fd,buf);
                        break;
                    case UNSHIELD:
                        Unshield_friend(fd,buf);
                        break;
                    case VIEW_CHAT_HISTORY:
                        View_chat_history(fd,buf);
                        break;
                    case CREAT_GROUP:
                        Create_group(fd,buf);
                        break;
                    case ADD_GROUP:
                        Add_group(fd,buf);
                        break;
                    case ADD_GROUP_ACCEPT:
                        Add_group_accept(fd,buf);
                        break;
                    case WITHDRAW_GROUP:
                        Withdraw_group(fd,buf);
                        break;
                    //一起实现
                    case VIEW_GROUP_MEMBER:
                        View_group_member(fd,buf);
                        break;
                    case VIEW_ADD_GROUP:
                        View_add_group(fd,buf);
                        break;
                    case SET_GROUP_ADMIN:
                        Set_group_admin(fd,buf);
                        break;
                    case KICK:
                        Kick(fd,buf);
                        break;
                    case GROUP_CHAT:
                        Group_chat(fd,buf);
                        break;
                    case VIEW_GROUP_RECORD:
                        View_group_record(fd,buf);
                        break;
                    /*case SEND_FILE:
                        printf("here\n");
                        {
                            printf("111111111111111\n");
                            char *buf1=(char*)malloc(sizeof(1024));
                            printf("22222222222222222\n");
                            memcpy(buf1,buf,sizeof(file_t));
                            printf("333333333333333333\n");
                            threadpool_add(Send_file,(void*)buf1);*/
                            /*printf("....\n");
                            file_t file;
                            char *str=(char*)malloc(sizeof(file));
                            memcpy(str,buf,sizeof(file));
                            pthread_t recv_file_id;
                            pthread_create(&recv_file_id,NULL,Send_file,(void*)str);
                            printf(">>>>\n");*/
                            
                            //Send_file(fd,buf);
                            //break;
                        //}
                    case SEND_FILE:
                        break;
                    case UPLOAD:
                        Upload(fd,buf);
                        break;;
                    /*case RECV_FILE:
                        //Recv_file(fd,buf);
                        break;*/
                    case DOWNLOAD:
                        Download(fd,buf);
                        break;
                    case 0:
                        break;
                }
            }
        }
    }
    close(epfd);
    close(lfd);
}

void Register(int fd,char* buf)
{
    Account_t account;
    memcpy(&account,buf,sizeof(account));
    printf("server name:%s\n",account.name);
    printf("server password:%s\n",account.password);
    server_user_t user;
    strcpy(user.name,account.name);
    strcpy(user.password,account.password);
    printf("server username:%s\n",user.name);
    printf("server password:%s\n",user.password);

    account.online=DOWNLINE;

   
    
    char buf_t[BUFSIZ];
    sprintf(buf_t,"insert into account values(NULL,'%s','%s','%d')",account.name,account.password,account.online);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("注册失败\n");
        /*char mes[256];
        sprintf(mes,"你输入的名称[%s]有误，请重新输入",account.name);
        Send_pack(fd,REGISTER_ERROR_APPLY,mes);*/
    }
    else 
        printf("注册成功\n");

    memset(buf_t,0,sizeof(buf_t));
    sprintf(buf_t,"select LAST_INSERT_ID()");
    printf("buf_t:%s\n",buf_t);
    ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    int id=atoi(row[0]);
    printf("server username:%d",id);
    char str[256];
    //strcpy(str,account.username);
    sprintf(str,"账号[%d]注册成功",id);
    Send_register_pack(fd,REGISTER_APPLY,str,id);
    printf("666666666\n");   
}               


void Send_register_pack(int fd,int flag,char* buf,int id)
{
    char str[BUFSIZ];
    message mes;

    mes.flag=flag;
    mes.id=id;
    strcpy(mes.message,buf);
    printf("server send message:%s\n",mes.message);
    memcpy(str,&mes,sizeof(mes));
    
    printf("server send id:%d\n",mes.id);
    printf("server send flag:%d\n",mes.flag);
    if(send(fd,str,sizeof(str),0)<0)
    {
        my_err("send error!",__LINE__);
        close(fd);
    }
}


void Login(int fd,char* buf)
{
    Account_t account;
    memcpy(&account,buf,sizeof(account));
    printf("server id:%d\n",account.id);
    printf("server password:%s\n",account.password);
    char name[MAX];
    
    
    int send_fd=Get_connfd(account.id);
    /*server_list_t pos;
    List_ForEach(list_ser,pos)
    {
        if(pos->data.id==account.id)
        {
            send_fd=pos->data.connfd;
        }
    }*/
    if(send_fd>0)
    {
        printf("账号已登录\n");
        Send_pack(fd,LOGIN_APPLY,"a");
        return ;
    }


    char buf_t[BUFSIZ];
    sprintf(buf_t,"select *from account where id=%d",account.id);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
        //printf("row[0]:%s\n",row[0]);
    if(row==NULL)
    {
        printf("/\n");
        /*char mes[256];
        sprintf(mes,"密码错误或者账号不存在!\n");
        Send_pack(fd,LOGIN_APPLY,mes);*/
        Send_pack(fd,LOGIN_APPLY,"n");
        return ;
    }
    
    else
    {
        printf("row[0]:%s\n",row[0]);
        //if(strcmp(row[0],user.username)==0)
        //{
          //  memset(buf_t,0,sizeof(buf_t));
            //sprintf(buf_t,"select username from account");
            //printf("buf_t:%s\n",buf_t);
            strcpy(name,row[1]);
            printf("name:%s\n",row[1]);
            if(strcmp(row[2],account.password)==0)
            {
                Add_node(fd,account.id,name);
                server_list_t p;
                List_ForEach(list_ser,p)
                {
                    printf("username:%d\n",p->data.id);
                    printf("connfd:%d\n",p->data.connfd);
                    printf("name:%s\n",p->data.name);
                }
                Send_pack(fd,LOGIN_APPLY,"y");
                memset(buf_t,0,sizeof(buf_t));
                sprintf(buf_t,"update account set status=1 where id=%d",account.id);
                printf("buf_t:%s\n",buf_t);
                int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
                if(ret)
                {
                    Mysql_with_error(&mysql);
                }
                printf("[%d]登录成功\n",account.id);
                Send_offline_apply(fd,account.id);
                Send_offline_messgae(fd,account.id);

            }
            else
                Send_pack(fd,LOGIN_APPLY,"n");
        //}
    }
}
void Send_offline_apply(int fd,int recver)
{
    char buf[BUFSIZ];
    sprintf(buf,"select *from friend where (fid=%d and  request=%d)",recver,0);
    printf("buf=%s\n",buf);
    int ret=mysql_real_query(&mysql,buf,strlen(buf));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("您暂无离线好友申请消息\n");
    }
    MYSQL_RES *result;
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    
    while((row=mysql_fetch_row(result)))
    {
        char buf[BUFSIZ];
        sprintf(buf,"update friend set request=2 where fid=%d",atoi(row[1]));
        printf("buf=%s\n",buf);
        int ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        char mes[256];
        int requester=atoi(row[0]);
        sprintf(mes,"用户[%d]请求加你为好友",requester);
        Send_connfd_pack(ADD_FRIEND_APPLY,requester,recver,mes);
        printf("[%d]对好友[%d]好友申请发送成功\n",requester,recver);
    }


    memset(buf,0,sizeof(buf));
    sprintf(buf,"select sid from group_request where (mid=%d and request=2)",recver);
    printf("buf=%s\n",buf);
    ret=mysql_real_query(&mysql,buf,strlen(buf));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result2;
    MYSQL_ROW  row2;
    result2=mysql_store_result(&mysql);
    //row2=mysql_fetch_row(result2);

   
    while((row2=mysql_fetch_row(result2)))
    {   
         printf("row2[0]:%d\n",atoi(row2[0]));
       
            char buf[BUFSIZ];

            char mes[256];
            int requester=atoi(row2[0]);
            sprintf(mes,"用户[%d]请求加入群聊",atoi(row2[0]));
            Send_connfd_pack(ADD_GROUP_APPLY,requester,recver,mes);
            printf("[%d]对群管理[%d]的群申请发送成功\n",requester,recver);


            sprintf(buf,"update group_member set request=1 where mid=%d",atoi(row2[0]));
            printf("buf:%s\n",buf);
            ret=mysql_real_query(&mysql,buf,strlen(buf));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }


             memset(buf,0,sizeof(buf));
            sprintf(buf,"update group_request set request=1 where mid=%d",recver);
            printf("buf:%s\n",buf);
            ret=mysql_real_query(&mysql,buf,strlen(buf));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }
    }

}
void Send_offline_messgae(int fd,int recver)
{
    char buf[BUFSIZ];
    memset(buf,0,sizeof(buf));
    sprintf(buf,"select *from chat_message where (fid=%d and status=%d)",recver,0);
    printf("buf=%s\n",buf);
    int ret=mysql_real_query(&mysql,buf,strlen(buf));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);

    while((row=mysql_fetch_row(result)))
    {

        char str[BUFSIZ];
        sprintf(str,"select *from account where id=%d",atoi(row[0]));
        printf("str:%s\n",str);
        int ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result1;
        MYSQL_ROW row1;
        result1=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result1);

        char name[MAX];
        strcpy(name,row1[1]);


        char buf[BUFSIZ];
        sprintf(buf,"update chat_message set status=2 where fid=%d",recver);
        printf("buf=%s\n",buf);
        ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        Chat_message mes_t;
        mes_t.flag=PRIVATE_CHAT_APPLY;
        mes_t.sender=atoi(row[0]);
        mes_t.recver=atoi(row[1]);
        strcpy(mes_t.message,row[2]);
        /*char str[BUFSIZ];
        memcpy(str,&mes_t,sizeof(mes_t));
        if(send(fd,buf,sizeof(buf),0)<0)
        {
            my_err("send error!",__LINE__);
        }*/
        char mes[256];
        sprintf(mes,"\033[;33m\33[1m%d\t\033[;32m\33[1m%s\t\033[;31m\33[1m%s\033[0m",mes_t.sender,name,mes_t.message);
        Send_pack(fd,PRIVATE_CHAT_APPLY,mes);
        printf("[%d]有对[%d]的离线聊天\n",mes_t.sender,mes_t.recver);
    }


    memset(buf,0,sizeof(buf));
    sprintf(buf,"select *from group_message where (recv_name=%d and status=0)",recver);
    printf("buf=%s\n",buf);
    ret=mysql_real_query(&mysql,buf,strlen(buf));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result1;
    MYSQL_ROW  row1;
    result1=mysql_store_result(&mysql);
    while((row1=mysql_fetch_row(result1)))
    {

        char str[BUFSIZ];
        sprintf(str,"select *from account where id=%d",atoi(row1[0]));
        printf("str:%s\n",str);
        int ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result2;
        MYSQL_ROW row2;
        result2=mysql_store_result(&mysql);
        row2=mysql_fetch_row(result2);

        char name[MAX];
        strcpy(name,row2[1]);




        Chat_message mes_t;
        mes_t.flag=GROUP_CHAT_APPLY;
        mes_t.sender=atoi(row1[0]);
        mes_t.recver=atoi(row1[1]);
        strcpy(mes_t.message,row1[2]);
        /*char str[BUFSIZ];
        memcpy(str,&mes_t,sizeof(mes_t));
        if(send(fd,buf,sizeof(buf),0)<0)
        {
            my_err("send error!",__LINE__);
        }*/
         char mes[256];
         sprintf(mes,"[%d]\t%s\t%s",mes_t.sender,name,mes_t.message);

         Send_pack(fd,GROUP_CHAT_APPLY,mes);
        printf("[%d]有对群的离线聊天\n",mes_t.sender);


        char buf[BUFSIZ];
        sprintf(buf,"update group_message set status=1 where recv_name=%d",recver);
        printf("buf=%s\n",buf);
        ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
    }


}
void Exit(int fd,char* buf)
{
    message mes;
    memcpy(&mes,buf,sizeof(mes));
    printf("server recv message:%s\n",mes.message);

    char buf_t[BUFSIZ];
    sprintf(buf_t,"update account set status=0 where id=%d",mes.id);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    printf("用户[%d]已经下线\n",mes.id);
    
    //char mes1[256];
    //sprintf(mes1,"您[%d]已退出",mes.id);
    //Send_pack(fd,EXIT_APPLY,mes1);

}

void Add_node(int fd,int id,char* name)
{
    server_list_t pos=(server_list_t)malloc(sizeof(server_user_node_t));
    pos->data.connfd=fd;
    pos->data.id=id;
    strcpy(pos->data.name,name);
    printf("pos->data.connfd:%d\n",pos->data.connfd);
    printf("pos->data.id=%d\n",pos->data.id);
    printf("pos->data.name=%s\n",pos->data.name);
    List_AddTail(list_ser,pos);
}
void Add_friend(int fd,char* buf)
{

    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ friend send:%d\n",relation.send);
    printf("server/ friend recv:%d\n",relation.recv);
    printf("server/ friend message:%s\n",relation.message);

    char buf_t[BUFSIZ];
    //查询有没有这两个人的账号
    sprintf(buf_t,"select request from friend where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    //获得申请者的客户端套接字
    int send_fd;
    send_fd=Get_connfd(relation.send);

    //如果没有这两个的信息
    if(row==NULL)
    {
        //查看添加好友账户是否存在
        char buf[BUFSIZ];
        sprintf(buf,"select status from account where id=%d",relation.recv);
        printf("buf:%s\n",buf);
        ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result1;  
        MYSQL_ROW  row1;
        result1=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result1);
        if(row1==NULL)
        {
            char mes[256];
            sprintf(mes,"账号[%d]不存在",relation.recv);
            Send_pack(send_fd,PRINT_APPLY,mes);
            return;
        }


        //用户存在
        relation.relation=STRANGER;
        memset(buf_t,0,sizeof(buf_t));
        //请求为0
        //表示好友还没验证
        sprintf(buf_t,"insert into friend values('%d','%d','%d','%d')",relation.send,relation.recv,relation.relation,0);
        printf("buf_t:%s\n",buf_t);
        ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        printf("用户%d对%d发起了好友申请\n",relation.send,relation.recv);

        //获得被申请好友的客户端套接字
        //如果在线
        int recv_fd=Get_connfd(relation.recv);
        printf("hehehheheheh>>>>>>>>>\n");
        printf("recv_fd:%d\n",recv_fd);
        if(recv_fd<0)
        {
            printf("所申请的用户[%d]不在线\n",relation.recv);
            Send_pack(send_fd,PRINT_APPLY,"d");
            return;
        }
        else
        {
            char str[256];
            sprintf(str,"用户[%d]向你发送了好友请求",relation.send);
            Send_connfd_pack(ADD_FRIEND_APPLY,relation.send,relation.recv,str);

            //如果在线为request为2已经发送
            char buf[BUFSIZ];
            sprintf(buf,"update friend set request=2 where (uid=%d and fid=%d)",relation.send,relation.recv);
            printf("buf=%s\n",buf);
            int ret=mysql_real_query(&mysql,buf,strlen(buf));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }
        }
    }
    else if(row!=NULL)
    {
        if(atoi(row[0])==0)
        {
            char mes[256];
            sprintf(mes,"正在等待对方验证，请不要重复发好友申请");
            Send_pack(send_fd,PRINT_APPLY,mes);
            return;
        }
        if(atoi(row[0])==2)
        {
            char mes[256];
            sprintf(mes,"请勿重复添加好友");
            Send_pack(send_fd,PRINT_APPLY,mes);
            return;
        }
    }


}
/*void Add_friend(int fd,char* buf)
{

    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ friend send:%d\n",relation.send);
    printf("server/ friend recv:%d\n",relation.recv);
    printf("server/ friend message:%s\n",relation.message);

    char buf_t[BUFSIZ];
    //查询有没有这两个人的账号
    sprintf(buf_t,"select *from friend where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    //获得申请者的客户端套接字
    int send_fd;
    send_fd=Get_connfd(relation.send);

    //如果没有这两个的信息
    if(row==NULL)
    {
        //
        relation.relation=STRANGER;
        memset(buf_t,0,sizeof(buf_t));
        sprintf(buf_t,"insert into friend values('%d','%d','%d','%d')",relation.send,relation.recv,relation.relation,0);
        printf("buf_t:%s\n",buf_t);
        ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        printf("用户%d对%d发起了好友申请\n",relation.send,relation.recv);

        //获得被申请好友的客户端套接字
        //如果在线
        int recv_fd=Get_connfd(relation.recv);
        printf("hehehheheheh>>>>>>>>>\n");
        printf("recv_fd:%d\n",recv_fd);
        if(recv_fd<0)
        {
            printf("所申请的用户[%d]不在线\n",relation.recv);
            Send_pack(send_fd,PRINT_APPLY,"d");
            return;
        }
        else
        {
            char str[256];
            sprintf(str,"用户[%d]向你发送了好友请求",relation.send);
            Send_connfd_pack(ADD_FRIEND_APPLY,relation.send,relation.recv,str);

            //如果在线为request为2已经发送
            char buf[BUFSIZ];
            sprintf(buf,"update friend set request=2 where (uid=%d and fid=%d)",relation.send,relation.recv);
            printf("buf=%s\n",buf);
            int ret=mysql_real_query(&mysql,buf,strlen(buf));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }
        }
    }
    else if(row!=NULL)
    {
        Send_pack(send_fd,PRINT_APPLY,"w");
        return;
    }


}*/
void Send_connfd_pack(int flag,int sender,int recver,char* buf)
{
    int recv_fd;
    server_list_t pos;
    //获得接受者套接字
    List_ForEach(list_ser,pos)
    {
        if(pos->data.id==recver)
        {
            printf("该用户的客户端套接字为:%d\n",pos->data.connfd);
            recv_fd=pos->data.connfd;
        }
    }
    int send_fd;
    send_fd=Get_connfd(sender);

    char str[BUFSIZ];
    box_t box;
    box.flag=flag;
    box.sender=sender;//申请者
    box.recver=recver;//被申请者
    box.send_fd=send_fd;//申请者客户端号
    box.recv_fd=recv_fd;//被申请者客户端号

    //buf:用户[%d]向你发送了好友请求
    strcpy(box.message,buf);

    memcpy(str,&box,sizeof(box));

    printf("server send message:%s\n",box.message);
    printf("server send flag:%d\n",box.flag);
    printf("server/ friend send id:%d\n",box.sender);
    printf("server/ friend recv id:%d\n",box.recver);
    //朋友客户端号
    printf("server/ friend recv_fd:%d\n",recv_fd);
    printf("server/ friend send_fd:%d\n",send_fd);

    if(send(recv_fd,str,sizeof(str),0)<0)
    {
        my_err("send error!",__LINE__);
    }

}
void Add_friend_accept(int fd,char* buf)
{
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ friend accpet send:%d\n",relation.send);
    printf("server/ friend accpet recv:%d\n",relation.recv);
    printf("server/ friend accpet message:%s\n",relation.message);

    char str1[BUFSIZ];
    char str2[BUFSIZ];
    if(strcmp(relation.message,"y")==0)
    {
        char buf[BUFSIZ];
        relation.relation=PAL;
        sprintf(buf,"update friend set status =%d where (uid=%d and fid=%d) ",relation.relation,relation.recv,relation.send);
        printf("buf:%s\n",buf);
        int ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        printf("[%d]成为了[%d]的新朋友\n",relation.send,relation.recv);

        sprintf(str1,"[%d]通过了您的好友请求",relation.send);
        Send_pack_name(ADD_FRIEND_ACCEPT_APPLY,relation.send,relation.recv,str1);//发给申请者

        sprintf(str2,"你已经和[%d]成为了朋友",relation.recv);
        Send_pack_name(ADD_FRIEND_ACCEPT_APPLY,relation.recv,relation.send,str2);//发给被申请者
    }
    else
    {
        char buf[BUFSIZ];
        sprintf(buf,"delete from friend where (uid=%d and fid=%d)",relation.recv,relation.send);
        printf("buf:%s\n",buf);
        int ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        printf("[%d]拒绝成为[%d]的好友\n",relation.send,relation.recv);

        memset(str1,0,sizeof(str1));
        sprintf(str1,"[%d]拒绝了您的好友请求",relation.send);
        Send_pack_name(ADD_FRIEND_ACCEPT_APPLY,relation.send,relation.recv,str1);//发给申请者

    }
}

void Send_pack_name(int flag ,int sender,int recver,char *buf)
{
    int recv_fd=Get_connfd(recver);
    char str[BUFSIZ];
    apply_messgae mes;

    mes.flag=flag;
    mes.sender=sender;//被申请者
    mes.recver=recver;//申请者
    strcpy(mes.message,buf);
    printf("server/ friend accept apply send:%d\n",mes.sender);
    printf("server/ friend accept apply recv:%d\n",mes.recver);
    printf("server/ friend accept apply send message:%s\n",mes.message);

    memcpy(str,&mes,sizeof(mes));
    
    printf("server send message:%s\n",mes.message);
    printf("server send flag:%d\n",mes.flag);
    if(send(recv_fd,str,sizeof(str),0)<0)
    {
        my_err("send error!",__LINE__);
    }


}
int Get_connfd(int id)
{
    server_list_t pos;
    List_ForEach(list_ser,pos)
    {
        if(pos->data.id==id)
        {
            printf("用户套接字:%d\n",pos->data.connfd);
            return pos->data.connfd;
        }
    }
    return -1;
}

void Del_friend(int fd,char* buf)
{
    
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ friend send:%d\n",relation.send);
    printf("server/ friend recv:%d\n",relation.recv);
    printf("server/ friend message:%s\n",relation.message);

     //获得发送者的端口号
    int send_fd=Get_connfd(relation.send);

    char buf_t[BUFSIZ];
    sprintf(buf_t,"select request from friend where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result; 
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        char mes[256];
        sprintf(mes,"好友[%d]不存在",relation.recv);
        Send_pack(send_fd,DEL_FRIEND_APPLY,mes);
        return;
    }
    if(atoi(row[0])==0)
    {
        char mes[256];
        sprintf(mes,"你和[%d]还不是好友,无法删除",relation.recv);
        Send_pack(send_fd,DEL_FRIEND_APPLY,mes);
        return;
    }



    memset(buf_t,0,sizeof(buf_t));
    //更新这两个人的关系
    sprintf(buf_t,"delete from friend where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("删除好友失败\n");
    }
    printf("[%d]删除了与[%d]的好友关系\n",relation.send,relation.recv);

   
    char str1[BUFSIZ];
    char str2[BUFSIZ];
    sprintf(str1,"你与[%d]的朋友关系已经解除",relation.recv);
    Send_pack(send_fd,DEL_FRIEND_APPLY,str1);

    //被删除者的端口号
    int recv_fd=Get_connfd(relation.recv);

    if(recv_fd<0)
    {
        char mes[256];
        sprintf(mes,"好友[%d]不在线",relation.recv);
        Send_pack(send_fd,DEL_FRIEND_APPLY,mes);
        return;
    }
    sprintf(str2,"你已经被[%d]删除了好友关系",relation.send);
    Send_pack(recv_fd,DEL_FRIEND_APPLY,str2);
}


/*void Del_friend(int fd,char* buf)
{
    
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ friend send:%d\n",relation.send);
    printf("server/ friend recv:%d\n",relation.recv);
    printf("server/ friend message:%s\n",relation.message);

    char buf_t[BUFSIZ];
    //更新这两个人的关系
    sprintf(buf_t,"delete from friend where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("删除好友失败\n");
    }
    printf("[%d]删除了与[%d]的好友关系\n",relation.send,relation.recv);

    //获得发送者的端口号
    int send_fd=Get_connfd(relation.send);
    char str1[BUFSIZ];
    char str2[BUFSIZ];
    sprintf(str1,"你与[%d]的朋友关系已经解除",relation.recv);
    Send_pack(send_fd,DEL_FRIEND_APPLY,str1);

    //被删除者的端口号
    int recv_fd=Get_connfd(relation.recv);

    if(recv_fd)
    {
        char mes[256];
        sprintf(mes,"好友[%d]不在线",relation.recv);
        Send_pack(send_fd,DEL_FRIEND_APPLY,mes);
        return;
    }
    sprintf(str2,"你已经被[%d]删除了好友关系",relation.send);
    Send_pack(recv_fd,DEL_FRIEND_APPLY,str2);


    //server_list_t pos;
    //server_list_t pos_friend;

    //pos=Find_server_user(pack_t->data.send_name);
    //Find_del_server_user(pos,pack_t->data.message);

    //pos_friend=Find_server_user(pack_t->data.message);
    //Find_del_server_user(pos_friend,pack_t->data.send_name);

    //free(pack_t);
}*/
int Check_relationship(int fd,int send,int recv)
{
    char str[BUFSIZ];
    sprintf(str,"select status from friend where (uid=%d and fid=%d and request=2) or (uid=%d and fid=%d and request=2)",send,recv,recv,send);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }

    MYSQL_RES *result; 
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    printf("here\n");
    if(row==NULL)
    {
        printf("why>\n");
        char mes[256];
        sprintf(mes,"[%d]还不是您的好友",recv);
        Send_pack(fd,PRIVATE_CHAT_APPLY,mes);
        //Send_pack(fd,PRIVATE_CHAT_APPLY,"0");
        return -1;
    }
    if(atoi(row[0])==2)
    {
        char mes[256];
        sprintf(mes,"[%d]已经被屏蔽",recv);
        Send_pack(fd,PRIVATE_CHAT_APPLY,mes);
        return 0;
    }
    return 1;
}
void Private_chat(int fd,char* buf)
{
    Chat_message mes;
    memcpy(&mes,buf,sizeof(mes));

    printf("server/ friend chat send:%d\n",mes.sender);
    printf("server/ friend chat recv:%d\n",mes.recver);
    //printf("server/ friend chat time:%s\n",mes.time);
    printf("server/ friend chat message:%s\n",mes.message);

    int flag=Check_relationship(fd,mes.sender,mes.recver);
    if(flag>0)
    {
        //先插入mysql
        char str[BUFSIZ];
        sprintf(str,"insert into chat_message values('%d','%d','%s','%d')",mes.sender,mes.recver,mes.message,0);
        //没发前状态为0
        printf("str:%s\n",str);
        int ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }

        printf("[%d]对[%d]聊天记录以及写入mysql\n",mes.sender,mes.recver);

        //获取聊天对方的套接字
        int recv_fd=Get_connfd(mes.recver);
        int send_fd=Get_connfd(mes.sender);
        if(recv_fd<0)
        {
            printf("[%d]不在线\n",mes.recver);
            char mes1[256];
            sprintf(mes1,"[%d]不在线",mes.recver);
            Send_pack(send_fd,PRIVATE_CHAT_APPLY,mes1);
            return;
        }
        else
        {
        
            char buf[BUFSIZ];
            sprintf(buf,"select *from chat_message where (uid=%d and fid=%d and status=%d)",mes.sender,mes.recver,0);//0未发送
            printf("buf:%s\n",buf);
            ret=mysql_real_query(&mysql,buf,strlen(buf));
            printf("?????\n");
            if(ret)
            {
                printf(">>>>>\n");
                Mysql_with_error(&mysql);
            }

            MYSQL_RES *result;
            MYSQL_ROW row;
            result=mysql_store_result(&mysql);

            
            while((row=mysql_fetch_row(result)))
            {
                //标志位改为1并且发送
                char str[BUFSIZ];
                sprintf(str,"update chat_message set status=%d where (uid=%d and fid=%d)",2,mes.sender,mes.recver);//2已经发送
                printf("str:%s\n",str);
                int ret=mysql_real_query(&mysql,str,strlen(str));
                if(ret)
                {
                    Mysql_with_error(&mysql);
                }
                else
                {
                    char str[BUFSIZ];
                    sprintf(str,"select *from account where id=%d",atoi(row[0]));
                    printf("str:%s\n",str);
                    int ret=mysql_real_query(&mysql,str,strlen(str));
                    if(ret)
                    {
                        Mysql_with_error(&mysql);
                    }
                    MYSQL_RES *result1;
                    MYSQL_ROW row1;
                    result1=mysql_store_result(&mysql);
                    row1=mysql_fetch_row(result1);

                    char name[MAX];
                    strcpy(name,row1[1]);

                    Chat_message mes_t;
                    mes_t.flag=PRIVATE_CHAT_APPLY;
                    mes_t.sender=atoi(row[0]);
                    mes_t.recver=atoi(row[1]);
                    //strcpy(mes_t.time,mes.time);
                    printf("row[2]:%s\n",row[2]);
                    strcpy(mes_t.message,row[2]);
                    printf("mes_t.flag:%d\n",mes_t.flag);
                    printf("mes_t.sender:%d\n",mes_t.sender);
                    printf("mes_t.recver:%d\n",mes_t.recver);
                    printf("mes_t.mes:%s\n",mes_t.message);
                    //printf("mes_t.time:%s\n",mes_t.time);
                   /* char buf[BUFSIZ];
                    memcpy(buf,&mes_t,sizeof(mes_t));
                    if(send(recv_fd,buf,sizeof(buf),0)<0)
                    {
                        my_err("send error!",__LINE__);
                    }*/
                    char mes[256];
                    sprintf(mes,"\033[;33m\33[1m%d\t\033[;32m\33[1m%s\t\033[;31m\33[1m%s\033[0m",mes_t.sender,name,mes_t.message);
                    Send_pack(recv_fd,PRIVATE_CHAT_APPLY,mes);
                    printf("[%d]正在对[%d]聊天\n",mes_t.sender,mes_t.recver);
                }  
            }  


        }
    }
    else if(flag==0)
    {
        printf("[%d]和[%d]处于屏蔽状态\n",mes.sender,mes.recver);
        char mes1[256];
        memset(&mes1,0,sizeof(mes1));
        sprintf(mes1,"你与[%d]处于屏蔽状态,无法聊天",mes.sender);
        Send_pack(fd,PRIVATE_CHAT_APPLY,mes1);
        return ;
    }

}
void View_chat_history(int fd,char* buf)
{
    Chat_message mes;
    memcpy(&mes,buf,sizeof(mes));

    char str[BUFSIZ];
    sprintf(str,"select *from friend where (uid=%d and fid=%d and request=2) or (uid=%d and fid=%d and request=2)",mes.sender,mes.recver,mes.recver,mes.sender);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        Send_pack(fd,VIEW_CHAT_HISTORY_APPLY,"你们还不是好友，请先添加好友聊天");
        printf("[%d]查询聊天记录失败\n",mes.sender);
        Send_pack(fd,VIEW_CHAT_HISTORY_APPLY,"over");
    }

    char str2[BUFSIZ];
    sprintf(str2,"select *from chat_message where (uid=%d and fid=%d and status=2) or (uid=%d and fid=%d and status=2)",mes.sender,mes.recver,mes.recver,mes.sender);
    printf("str2:%s\n",str2);
    ret=mysql_real_query(&mysql,str2,strlen(str2));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }

    int send_fd=Get_connfd(mes.sender);
    char str1[256];
    printf("22222222\n");
    MYSQL_RES *result2;  
    MYSQL_ROW  row2;
    result2=mysql_store_result(&mysql);
    while((row2=mysql_fetch_row(result2)))
    {
         char str1[256];
         memset(str1,0,sizeof(str1));
        printf("1111111\n");
        sprintf(str1,"[%d]对[%d]:%s",atoi(row2[0]),atoi(row2[1]),row2[2]);
        Send_pack(send_fd,VIEW_CHAT_HISTORY_APPLY,str1);
    }
    Send_pack(fd,VIEW_CHAT_HISTORY_APPLY,"over");

    printf("[%d]查询了与[%d]的聊天记录\n",mes.sender,mes.recver);


}

void Shield_friend(int fd,char* buf)
{
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ friend send:%d\n",relation.send);
    printf("server/ friend recv:%d\n",relation.recv);
    printf("server/ friend message:%s\n",relation.message);


    //获得发送者的端口号
    int send_fd=Get_connfd(relation.send);

    char buf_t[BUFSIZ];
    sprintf(buf_t,"select request,status from friend where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
     MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        char mes[256];
        sprintf(mes,"账号[%d]不存在",relation.recv);
        Send_pack(send_fd,SHIELD_APPLY,mes);
        return;
    }
    if(atoi(row[0])==0)
    {
        char mes[256];
        sprintf(mes,"对方还不是你的好友,操作无效");
        Send_pack(send_fd,SHIELD_APPLY,mes);
        return;
    }
    if(atoi(row[1])==2)
    {
        char mes[256];
        sprintf(mes,"你已经屏蔽了好友[%d],请勿重复操作",relation.recv);
        Send_pack(send_fd,SHIELD_APPLY,mes);
        return;
    }



    memset(buf_t,0,sizeof(buf_t));
    //更新这两个人的关系
    relation.relation=BLACK;
    sprintf(buf_t,"update friend set status=2 where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("屏蔽好友失败\n");
    }
    printf("[%d]屏蔽了与[%d]的好友关系\n",relation.send,relation.recv);

    

   

    char str1[256];
    sprintf(str1,"你已经屏蔽了[%d]",relation.recv);
    Send_pack(send_fd,SHIELD_APPLY,str1);

    int recv_fd=Get_connfd(relation.recv);

    if(recv_fd<0)
    {
        char mes[256];
        sprintf(mes,"屏蔽的好友[%d]不在线",relation.recv);
        Send_pack(send_fd,SHIELD_APPLY,mes);
        return;
    }
    printf("????\n");
    char str2[256];
    memset(str2,0,sizeof(str2));
    sprintf(str2,"你被[%d]屏蔽了",relation.send);
    Send_pack(recv_fd,SHIELD_APPLY,str2);

}
void Unshield_friend(int fd,char* buf)
{
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ friend send:%d\n",relation.send);
    printf("server/ friend recv:%d\n",relation.recv);
    printf("server/ friend message:%s\n",relation.message);

    //获得发送者的端口号
    int send_fd=Get_connfd(relation.send);



    char buf_t[BUFSIZ];
    sprintf(buf_t,"select request,status from friend where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    int ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
     MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        char mes[256];
        sprintf(mes,"账号[%d]不存在",relation.recv);
        Send_pack(send_fd,UNSHIELD_APPLY,mes);
        return;
    }
    if(atoi(row[0])==0)
    {
        char mes[256];
        sprintf(mes,"对方还不是你的好友,操作无效");
        Send_pack(send_fd,UNSHIELD_APPLY,mes);
        return;
    }
    if(atoi(row[1])==1)
    {
        char mes[256];
        sprintf(mes,"[%d]已经被解除了屏蔽,操作无效",relation.recv);
        Send_pack(send_fd,UNSHIELD_APPLY,mes);
        return;
    }


    memset(buf_t,0,sizeof(buf_t));
    //更新这两个人的关系
    relation.relation=PAL;
    sprintf(buf_t,"update friend set status=1 where (uid=%d and fid=%d) or (uid=%d and fid=%d)",relation.send,relation.recv,relation.recv,relation.send);
    printf("buf_t:%s\n",buf_t);
    ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("解除屏蔽失败\n");
    }
    printf("[%d]成功解除了屏蔽与[%d]的好友关系\n",relation.send,relation.recv);

   
    char str1[256];
    sprintf(str1,"你已经成功解除屏蔽[%d]",relation.recv);
    Send_pack(send_fd,UNSHIELD_APPLY,str1);

    int recv_fd=Get_connfd(relation.recv);
    if(recv_fd<0)
    {
         printf("[%d]不在线\n",relation.recv);
            char mes1[256];
            sprintf(mes1,"[%d]不在线",relation.recv);
            Send_pack(send_fd,UNSHIELD_APPLY,mes1);
        return;
    }
    char str2[256];
    sprintf(str2,"你被[%d]解除了屏蔽",relation.send);
    Send_pack(recv_fd,SHIELD_APPLY,str2);
}

//一起实现
//void Show_friend_status();
void View_friend_list(int fd,char* buf)
{
    Friend_t friend;
    memcpy(&friend,buf,sizeof(friend));

    printf("server/ friend list send:%d\n",friend.send);
    printf("server/ friend list recv:%d\n",friend.recv);
    printf("server/ friend list message:%s\n",friend.message);


    char str[BUFSIZ];
    sprintf(str,"select *from friend where (uid=%d or fid=%d)",friend.send,friend.recv);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("[%d]查询好友失败\n",friend.send);
    }




    //Friend_t send_friend;
    int send_fd=Get_connfd(friend.send);
    int id;
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    while((row=mysql_fetch_row(result)))
    {
        if(atoi(row[0])==friend.send)
            id=atoi(row[1]);
        else
            id=atoi(row[0]);
    
        printf("id:%d\n",id);
        int status;
        /*server_list_t pos;
        List_ForEach(list_ser,pos)
        {
            if(pos->data.id==id)
            {
                send_friend.status=ONLINE;
                strcpy(send_friend.name,pos->data.name);
                printf("000000000000000");
                printf("这是在线\n");
            }
            else
            {
                send_friend.status=DOWNLINE;
                 strcpy(send_friend.name,pos->data.name);
                 printf("1111111111111111离线\n");
            }
        }*/
       

        char buff[BUFSIZ];
        sprintf(buff,"select *from account where id=%d",id);
        printf("buff:%s\n",buff);
        int ret=mysql_real_query(&mysql,buff,strlen(buff));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
       MYSQL_RES *result1;  
        MYSQL_ROW  row1;
        result1=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result1);
        status=atoi(row1[3]);
        char name[MAX];
        strncpy(name,row1[1],sizeof(row[1]));
        


        printf("1111111111111111111name:%s\n",name);
        if(atoi(row[2])==1)
        {
            printf("row[2]:%d\n",atoi(row[2]));
            if(status==1)
            {
                sprintf(str,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\t\033[;34m\33[1m%s\033[0m",id,name,"在线","好友");
                Send_pack(send_fd,VIEW_FRIEND_LIST_APPLY,str);
            }
            else 
            {
                sprintf(str,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\t\033[;34m\33[1m%s\033[0m",id,name,"离线","好友");
                Send_pack(send_fd,VIEW_FRIEND_LIST_APPLY,str);
            }
            
        }
        else if(atoi(row[2])==2)
        {
            printf("row[2]:%d\n",atoi(row[2]));
            if(status==1)
            {
                sprintf(str,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\t\033[;34m\33[1m%s\033[0m",id,name,"在线","黑名单");
                Send_pack(send_fd,VIEW_FRIEND_LIST_APPLY,str);
            }
            else
            {
                sprintf(str,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\t\033[;34m\33[1m%s\033[0m",id,name,"离线","黑名单");
                Send_pack(send_fd,VIEW_FRIEND_LIST_APPLY,str);
            }
        }
    }
    Send_pack(fd,VIEW_FRIEND_LIST_APPLY,"over");
    printf("[%d]查询好友成功\n",friend.send);
}
    
int Get_status(int id)
{
    server_list_t pos;
        List_ForEach(list_ser,pos)
        {
            if(pos->data.id==id)
            {
                return 1;
                printf("000000000000000");
                printf("这是在线\n");
            }
        }
        return 0;
}
char* Get_name(int id)
{
    server_list_t pos;
        List_ForEach(list_ser,pos)
        {
            if(pos->data.id==id)
            {
                return pos->data.name;
                printf("000000000000000");
                printf("这是在线\n");
            }
        }
        return NULL;
}
void Create_group(int fd,char* buf)
{

    Group_t group;
    memcpy(&group,buf,sizeof(group));
    printf("server/ create group_name:%s\n",group.group_name);
    printf("server/ create group_id:%d\n",group.group_owner);
    printf("server/ create group_owner:%d\n",group.group_owner);
    printf("server/ create group_admin:%d\n",group.admin);

//查看群号是否被注册过
    char str1[BUFSIZ];
    sprintf(str1,"select gid from groups where group_name='%s'",group.group_name);
    printf("str1:%s\n",str1);
    int ret=mysql_real_query(&mysql,str1,strlen(str1));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;
    MYSQL_ROW row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);


    int send_fd=Get_connfd(group.group_owner);
    printf("send_fd:%d\n",send_fd);
    if(row!=NULL)
    {
        char mes[256];
        sprintf(mes,"该群名[%s]的群号[%d]已被注册过，请重新创建群",group.group_name,group.group_owner);
        Send_pack(send_fd,CREAT_GROUP_APPLY,mes);
        return;
    }


    char str[BUFSIZ];
    sprintf(str,"insert into groups values(NULL,'%s')",group.group_name);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("注册群失败\n");
    }

    char buf_t[BUFSIZ];
    sprintf(buf_t,"select LAST_INSERT_ID()");
    printf("buf_t:%s\n",buf_t);
    ret=mysql_real_query(&mysql,buf_t,strlen(buf_t));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result1;  
    MYSQL_ROW  row1;
    result1=mysql_store_result(&mysql);
    row1=mysql_fetch_row(result1);
    int id=atoi(row1[0]);
    printf("group_id:%d",id);


    char str2[256];
    sprintf(str2,"群名[%s]----群号[%d]创建成功",group.group_name,id);
    printf("str2:%s\n",str2);
    Send_pack(send_fd,CREAT_GROUP_APPLY,str2);


    memset(str,0,sizeof(str));
    //群号  群成员 群地位 请求
    sprintf(str,"insert into group_member values('%d','%d','%d','%d')",id,group.group_owner,OWNER,1);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    printf("[%d]注册群成功\n",group.group_owner);

   
}
void Add_group(int fd,char* buf)
{
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));

    
    char str[BUFSIZ];
    int send_fd=Get_connfd(relation.send);

    //查询是否申请过
    char str1[BUFSIZ];
    sprintf(str1,"select request from group_member where gid=%d and mid=%d",relation.recv,relation.send);
    printf("str1:%s\n",str1);
    int ret=mysql_real_query(&mysql,str1,strlen(str1));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row)
    {
        //已经是成员
       if(atoi(row[0])==1)
       {
           char mes[256];
           sprintf(mes,"您已经是群[%d]成员,请勿重复添加",relation.recv);
           Send_pack(send_fd,GROUP_APPLY,mes);
       }
       //重复添加
       else if(atoi(row[0])==2)
       {
            char mes[256];
           sprintf(mes,"您的加群[%d]正在审核,请勿重复添加",relation.recv);
           Send_pack(send_fd,GROUP_APPLY,mes);
       }
       return ;
    }



    //查询是否有该群
    memset(str,0,sizeof(str));
    sprintf(str,"select group_name from groups where gid=%d",relation.recv);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result1;
    MYSQL_ROW  row1;
    result1=mysql_store_result(&mysql);
    row1=mysql_fetch_row(result1);
    if(row1==NULL)
    {
        char mes[256];
        sprintf(mes,"此群[%d]不存在",relation.recv);
        Send_pack(send_fd,GROUP_APPLY,mes);
        return;
    }

    //群存在
    //先插入数据到 group_member
    //待审核 request 2
    //群号 群成员 请求
    memset(str,0,sizeof(str));
    sprintf(str,"insert into group_member(gid,mid,request) values('%d','%d','%d')",relation.recv,relation.send,2);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    printf("[%d]请求加入群[%d]\n",relation.send,relation.recv);


    //查询管理人员

    memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where gid=%d and (status=1 or status=2)",relation.recv);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    
    MYSQL_RES *result2;
    MYSQL_ROW  row2;
    result2=mysql_store_result(&mysql);
    while((row2=mysql_fetch_row(result2)))
    {
        memset(str,0,sizeof(str));
        //请求为2  待审核
        sprintf(str,"insert into group_request values(%d,%d,%d)",relation.send,atoi(row2[0]),2);
        printf("str:%s\n",str);
        ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        /*memset(str,0,sizeof(str));
        sprintf(str,"select status from account where id=%d",atoi(row2[0]));
        printf("str:%s\n",str);
        ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result3;
        MYSQL_ROW  row3;
        result3=mysql_store_result(&mysql);
        row3=mysql_fetch_row(result3);*/
        int recv_fd=Get_connfd(atoi(row2[0]));

        if(recv_fd<0)
        {
             printf("群[%d]管理不在线,请等待审核\n",relation.recv);
            char mes[256];
            sprintf(mes,"群[%d]管理不在线,请等待审核",relation.recv);
            Send_pack(send_fd,GROUP_APPLY,mes);
            return;
        }

         printf("[%d]向群管理[%d]发起了群申请\n",relation.send,atoi(row2[0]));
        //群管理处理
        char str2[256];
        sprintf(str2,"用户[%d]向你(群管理[%d])发送了群申请",relation.send,atoi(row2[0]));
        printf("str2:%s\n",str2);
        Send_connfd_pack(ADD_GROUP_APPLY,relation.send,atoi(row2[0]),str2);



        //如果在线为request为2已经发送
            char buf[BUFSIZ];
            sprintf(buf,"update group_member set request=1 where (gid=%d and mid=%d)",relation.recv,relation.send);
            printf("buf=%s\n",buf);
            int ret=mysql_real_query(&mysql,buf,strlen(buf));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }

        memset(buf,0,sizeof(buf));
            sprintf(buf,"update group_request set request=1 where (sid=%d and mid=%d)",relation.send,atoi(row2[0]));
            printf("buf=%s\n",buf);
            ret=mysql_real_query(&mysql,buf,strlen(buf));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }
    }
}
   

    
    
    /*//查询管理人员
    //找到群主
    memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where gid=%d and status=1",relation.recv);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    
    MYSQL_RES *result2;
    MYSQL_ROW  row2;
    result2=mysql_store_result(&mysql);
    row2=mysql_fetch_row(result2);
    
    Group_t group;
    group.group_owner=atoi(row2[0]);

    int owner_fd=Get_connfd(group.group_owner);


    //群管理
    memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where gid=%d and status=2",relation.recv);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result3;
    MYSQL_ROW  row3;
    result3=mysql_store_result(&mysql);
    row3=mysql_fetch_row(result3);
    if(row3==NULL)
    {
        printf("群[%d]暂时没有管理员\n",relation.recv);
          printf("[%d]向群主[%d]发起了群申请\n",relation.send,group.group_owner);


        if(owner_fd<0)
        {
            //请求为2  待审核
            char str[BUFSIZ];
            sprintf(str,"insert into group_request values(%d,%d,%d)",relation.send,group.group_owner,2);
            printf("str:%s\n",str);
            ret=mysql_real_query(&mysql,str,strlen(str));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }

            printf("群[%d]主不在线,请等待审核\n",relation.recv);
            char mes[256];
            sprintf(mes,"群[%d]主不在线,请等待审核",relation.recv);
            Send_pack(send_fd,GROUP_APPLY,mes);
            return;
        }


        //群主处理
        char str2[256];
        sprintf(str2,"用户[%d]向你(群主[%d])发送了群申请",relation.send,group.group_owner);
        printf("str2:%s\n",str2);
        Send_connfd_pack(ADD_GROUP_APPLY,relation.send,group.group_owner,str2);
        return ;
    }


    //如果有管理员
    printf("row3:%d\n",atoi(row3[0]));
    group.admin=atoi(row3[0]);


    int admin_fd=Get_connfd(group.admin);

    if(owner_fd<0)
    {
        printf("群[%d]主不在线,请等待审核\n",relation.recv);
            char mes[256];
            sprintf(mes,"群[%d]主不在线,请等待审核",relation.recv);
            Send_pack(send_fd,GROUP_APPLY,mes);
            return;
    }
    else if(admin_fd<0)
    {
        
          printf("群[%d]管理不在线,请等待审核\n",relation.recv);
            char mes[256];
            sprintf(mes,"群[%d]管理不在线,请等待审核",relation.recv);
            Send_pack(send_fd,GROUP_APPLY,mes);
            return;
    }
    else if(owner_fd<0 && admin_fd<0)
    {
         printf("群[%d]管理不在线,请等待审核\n",relation.recv);
            char mes[256];
            sprintf(mes,"群[%d]管理不在线,请等待审核",relation.recv);
            Send_pack(send_fd,GROUP_APPLY,mes);
            return;
    }
      //群管理处理
        printf("[%d]向群管理[%d]发送了群申请\n",relation.send,group.admin);
        char str3[256];
        sprintf(str3,"用户[%d]向你(群管理[%d])发送了群申请",relation.send,group.admin);
        printf("str3:%s\n",str3);
        Send_connfd_pack(ADD_GROUP_APPLY,relation.send,group.admin,str3);
*/
void Add_group_accept(int fd,char* buf)
{
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    printf("server/ group accpet send:%d\n",relation.send);//管理员
    printf("server/ group accpet recv:%d\n",relation.recv);//申请者
    printf("server/ group accpet message:%s\n",relation.message);

    char str1[BUFSIZ];
    char str2[BUFSIZ];
    if(strcmp(relation.message,"y")==0)
    {
        //更改地位
        char buf[BUFSIZ];
        relation.relation=COMMON;
        sprintf(buf,"update group_member set status=%d where mid=%d ",relation.relation,relation.recv);
        printf("buf:%s\n",buf);
        int ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        //更改请求
        memset(buf,0,sizeof(buf));
        sprintf(buf,"update group_member set request=%d where mid=%d ",1,relation.recv);
        printf("buf:%s\n",buf);
        ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        printf("[%d]同意了[%d]的入群请求\n",relation.send,relation.recv);

        sprintf(str1,"[%d]通过了您的入群请求",relation.send);
        Send_pack_name(ADD_GROUP_ACCEPT_APPLY,relation.send,relation.recv,str1);//发给申请者

        sprintf(str2,"您已经同意了[%d]的入群请求",relation.recv);
        Send_pack_name(ADD_GROUP_ACCEPT_APPLY,relation.recv,relation.send,str2);//发给管理员
    }
    else
    {
        char buf[BUFSIZ];
        sprintf(buf,"delete from group_member where mid=%d",relation.recv);
        printf("buf:%s\n",buf);
        int ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        printf("[%d]拒绝了[%d]的入群请求\n",relation.send,relation.recv);

        memset(str1,0,sizeof(str1));
        sprintf(str1,"[%d]拒绝了您的入群请求",relation.send);
        Send_pack_name(ADD_FRIEND_ACCEPT_APPLY,relation.send,relation.recv,str1);//发给申请者

    }
}
void Withdraw_group(int fd,char* buf)
{
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));
    
    //先查询是否是群成员
    char str1[BUFSIZ];
    sprintf(str1,"select request from group_member where (gid=%d and mid=%d)",relation.recv,relation.send);
    printf("str1:%s\n",str1);
    int ret=mysql_real_query(&mysql,str1,strlen(str1));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        char mes[256];
        sprintf(mes,"群[%d]不存在",relation.recv);
        Send_pack(fd,WITHDRAW_GROUP_APPLY,mes);
        return;
    }


    if(atoi(row[0])==2)
    {
        char mes[256];
        sprintf(mes,"你不在该群[%d]中,无法退出(没有被审核)",relation.recv);
        Send_pack(fd,WITHDRAW_GROUP_APPLY,mes);
        return;
    }
    
        //查询群主
        Group_t group;
        //memset(str,0,sizeof(str));
        char str[BUFSIZ];
        sprintf(str,"select mid from group_member where (gid=%d and status=1)",relation.recv);
        printf("str:%s\n",str);
        ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result1;
        MYSQL_ROW  row1;
        result1=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result1);
        group.group_owner=atoi(row1[0]);

        int send_fd=Get_connfd(group.group_owner);

         //获得申请退群者的端口号
        int fd_t=Get_connfd(relation.send);

        if(send_fd<0)
        {
            char mes[256];
            sprintf(mes,"[%d]不在线,请稍后再试",group.group_owner);
            Send_pack(fd_t,WITHDRAW_GROUP_APPLY,mes);
            return;
        }

        char mes[256];
        printf("[%d]退出了群聊[%d]\n",relation.send,relation.recv);
        sprintf(mes,"[%d]退出了你的群聊[%d]",relation.send,relation.recv);
        Send_pack(send_fd,WITHDRAW_GROUP_APPLY,mes);



        memset(str,0,sizeof(str));
        sprintf(str,"delete from group_member where (mid=%d and gid=%d)",relation.send,relation.recv);
        printf("str:%s\n",str);
        ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }

       
        memset(mes,0,sizeof(mes));
        sprintf(mes,"您已经退出了群聊[%d]\n",relation.recv);
        printf("mes:%s\n",mes);
        Send_pack(fd_t,WITHDRAW_GROUP_APPLY,mes);
    
        //如果群主解散
        if(relation.send==group.group_owner)
        {
            char str1[BUFSIZ];
            sprintf(str1,"delete from groups where gid=%d",relation.recv);
            printf("str1:%s\n",str1);
            ret=mysql_real_query(&mysql,str1,strlen(str1));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }
            
            memset(str1,0,sizeof(str1));
            sprintf(str1,"delete from group_member where gid=%d",relation.recv);
            printf("str1:%s\n",str1);
            ret=mysql_real_query(&mysql,str1,strlen(str1));
            if(ret)
            {
                Mysql_with_error(&mysql);
            }
            

            char str2[256];
            sprintf(str2,"群[%d]已经解散",relation.recv);
            Send_pack(send_fd,DEL_GROUP_APPLY,str2);
            return ;
        }

}
void View_group_member(int fd,char* buf)
{
     Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));

    char str[BUFSIZ];
    sprintf(str,"select *from group_member where (mid=%d and gid=%d and request=1)",relation.send,relation.recv);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("[%d]查询群信息失败\n",relation.send);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    printf("111111111111\n");
    int send_fd=Get_connfd(relation.send);
    if(row==NULL)
    {
         printf("你[%d]还没有加入此群或者此群不存在\n",relation.send);
        char mes[256];
        sprintf(mes,"你还没有加入此群聊或此群不存在");
        Send_pack(send_fd,VIEW_GROUP_MEMBER_APPLY,mes);
         Send_pack(fd,VIEW_GROUP_MEMBER_APPLY,"over");
        return ;
    }

    //查找成员
    memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where (gid=%d and request=1)",relation.recv);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result1;  
    MYSQL_ROW  row1;
    result1=mysql_store_result(&mysql);
   
    while((row1=mysql_fetch_row(result1)))
    {

        char buf[BUFSIZ];
 
        memset(buf,0,sizeof(buf));
        sprintf(buf,"select username,status from account where id=%d",atoi(row1[0]));
        printf("buf:%s\n",buf);
        ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result2;  
        MYSQL_ROW  row2;
        result2=mysql_store_result(&mysql);
        row2=mysql_fetch_row(result2);

        char mes[256];
        memset(&mes,0,sizeof(mes));
        //账号 名字 状态
        if(atoi(row2[1])==1)
            sprintf(mes,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\033[0m",atoi(row1[0]),row2[0],"在线");
        else if(atoi(row2[1])==0)
            sprintf(mes,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\033[0m",atoi(row1[0]),row2[0],"离线");
        Send_pack(send_fd,VIEW_GROUP_MEMBER_APPLY,mes);
         //Send_pack(fd,VIEW_GROUP_MEMBER_APPLY,"over");
    }

    Send_pack(fd,VIEW_GROUP_MEMBER_APPLY,"over");
}
/*

char member[MAX_CHAR][MAX_CHAR];

    int i=0;
    int j=0;
while((row1=mysql_fetch_row(result1)))
    {
        strcpy(member[i],row1[0]);
        i++;
    }
for(j=0;j<i;j++)
    {
        char buf[BUFSIZ];
        memset(buf,0,sizeof(buf));
        sprintf(buf,"select username,status from account where id=%d",atoi(member[j]));
        printf("buf:%s\n",buf);
        ret=mysql_real_query(&mysql,buf,strlen(buf));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result2;  
        MYSQL_ROW  row2;
        result2=mysql_store_result(&mysql);
        row2=mysql_fetch_row(result2);

        char mes[256];
        memset(&mes,0,sizeof(mes));
        //账号 名字 状态
        if(atoi(row2[1])==1)
            sprintf(mes,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\033[0m",atoi(member[j]),row2[0],"在线");
        else if(atoi(row2[1])==0)
            sprintf(mes,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;35m\33[1m%s\033[0m",atoi(member[j]),row2[0],"离线");
        Send_pack(send_fd,VIEW_GROUP_MEMBER_APPLY,mes);
    }*/
void View_add_group(int fd,char* buf)
{
    Relation_t relation;
    memcpy(&relation,buf,sizeof(relation));

    char str[BUFSIZ];
    memset(str,0,sizeof(str));
    sprintf(str,"select gid,mid,status from group_member where (mid=%d and request=1)",relation.send);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("[%d]查询群信息失败\n",relation.send);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    printf("111111111111\n");
    int send_fd=Get_connfd(relation.send);
    //if((row=mysql_fetch_row(result))==NULL)
    if(row==NULL)
    {
        printf("2222222222\n");
         printf("你[%d]还没有加入任何群\n",relation.send);
        char mes[256];
         memset(&mes,0,sizeof(mes));
        sprintf(mes,"你还没有加入任何群聊");
        Send_pack(send_fd,VIEW_ADD_GROUP_APPLY,mes);
         Send_pack(fd,VIEW_ADD_GROUP_APPLY,"over");
        return ;
    }
    printf("44444444\n");
    


    memset(str,0,sizeof(str));
    sprintf(str,"select gid,mid,status from group_member where (mid=%d and request=1)",relation.send);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("[%d]查询群信息失败\n",relation.send);
    }
    MYSQL_RES *result2;  
    MYSQL_ROW  row2;
    result2=mysql_store_result(&mysql);
    while((row2=mysql_fetch_row(result2)))
    {    
        printf("3333333333333\n");
        char str[BUFSIZ];
        memset(str,0,sizeof(str));
        sprintf(str,"select group_name from groups where gid=%d",atoi(row2[0]));
        printf("str:%s\n",str);
        ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
            printf("[%d]查询群信息失败\n",relation.send);
        }
        MYSQL_RES *result1;  
        MYSQL_ROW  row1;
        result1=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result1);
        

        char mes[256];
        memset(&mes,0,sizeof(mes));
        //群号 群名  地位
        if(atoi(row2[2])==1)
        {
            sprintf(mes,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;36m\33[1m%s\033[0m",atoi(row2[0]),row1[0],"群主");
            Send_pack(fd,VIEW_ADD_GROUP_APPLY,mes);
        }
        else if(atoi(row2[2])==2)
        {     
            sprintf(mes,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;36m\33[1m%s\033[0m",atoi(row2[0]),row1[0],"群管理");
            Send_pack(fd,VIEW_ADD_GROUP_APPLY,mes);
        }
        else if(atoi(row2[2])==3)
        {
             sprintf(mes,"\033[;33m\33[1m%d\t\033[;31m\33[1m%s\t\033[;36m\33[1m%s\033[0m",atoi(row2[0]),row1[0],"群成员");
            Send_pack(fd,VIEW_ADD_GROUP_APPLY,mes);
        }
        
    }
    //while((row=mysql_fetch_row(result)));
    
    Send_pack(fd,VIEW_ADD_GROUP_APPLY,"over");

    printf("666666666666\n");
    printf("[%d]查询群信息成功\n",relation.send);

}

void Group_chat(int fd,char* buf)
{
    Chat_message mes;
    memcpy(&mes,buf,sizeof(mes));
    printf("mes.send= %d\n",mes.sender);
    printf("mes.recv= %d\n",mes.recver);
    printf("mes.message= %s\n",mes.message);


    //先判断有没有这个群
    char str3[BUFSIZ];
    memset(str3,0,sizeof(str3));
    sprintf(str3,"select group_name from groups where gid=%d",mes.recver);
    printf("str3:%s\n",str3);
    int ret=mysql_real_query(&mysql,str3,strlen(str3));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result4;  
    MYSQL_ROW  row4;
    result4=mysql_store_result(&mysql);
    row4=mysql_fetch_row(result4);
    if(row4==NULL)
    {
        char mes1[256];
        memset(&mes1,0,sizeof(mes1));
        sprintf(mes1,"[%d]群聊不存在",mes.recver);
        Send_pack(fd,GROUP_CHAT_APPLY,mes1);
        return ;
    }

    //找出发送者姓名
    memset(str3,0,sizeof(str3));
    sprintf(str3,"select username from account where id=%d",mes.sender);
    printf("str3:%s\n",str3);
    ret=mysql_real_query(&mysql,str3,strlen(str3));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result3;  
    MYSQL_ROW  row3;
    result3=mysql_store_result(&mysql);
    row3=mysql_fetch_row(result3);

    char name[MAX];
    strcpy(name,row3[0]);

//是否是群成员
//1为已加群
    char str[BUFSIZ];
    memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where (gid=%d and request=1)",mes.recver);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        char mes1[256];
        memset(&mes1,0,sizeof(mes1));
        sprintf(mes1,"你还不是该群[%d]成员",mes.recver);
        Send_pack(fd,GROUP_CHAT_APPLY,mes1);
        return ;
    }


    memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where (gid=%d and mid!=%d)",mes.recver,mes.sender);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result2;  
    MYSQL_ROW  row2;
    result2=mysql_store_result(&mysql);
    while((row2=mysql_fetch_row(result2)))
    {
        //先插入mysql
        char str1[BUFSIZ];
        //0未发送
        //发送者  接受者  信息  状态 群号
        sprintf(str1,"insert into group_message values('%d','%d','%s','%d','%d')",mes.sender,atoi(row2[0]),mes.message,0,mes.recver);

        printf("str1:%s\n",str1);
        ret=mysql_real_query(&mysql,str1,strlen(str1));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }

        printf("[%d]对[%d]群聊成员[%d]的聊天记录以及写入mysql\n",mes.sender,mes.recver,atoi(row2[0]));

        /*char str2[BUFSIZ];
        sprintf(str2,"select status from account where id=%d",atoi(row[0]));
        ret=mysql_real_query(&mysql,str2,strlen(str2));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
        MYSQL_RES *result1;  
        MYSQL_ROW  row1;
        result1=mysql_store_result(&mysql);
        row1=mysql_fetch_row(result1);*/

        int send_fd=Get_connfd(atoi(row2[0]));
        //不在线
        /*if(atoi(row1[0])==0)
        {
            continue;
        }*/
        if(send_fd<0)
        {
            continue;
        }
        //在线
        char mes1[256];
        memset(&mes1,0,sizeof(mes1));
        //发送者账号 名字 信息
        sprintf(mes1,"[%d]\t%s\t%s",mes.sender,name,mes.message);
        Send_pack(send_fd,GROUP_CHAT_APPLY,mes1);
        

   
   
        char str3[BUFSIZ];
        //已发改为1
        sprintf(str3,"update group_message set status=1 where (send_name=%d and recv_name=%d and message=%s and gid=%d)",mes.sender,atoi(row2[0]),mes.message,mes.recver);
        ret=mysql_real_query(&mysql,str3,strlen(str3));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
   
        printf("[%d]正在群[%d]聊天\n",mes.sender,mes.recver);
    }

}
void View_group_record(int fd,char* buf)
{
    Chat_message mes;
    memcpy(&mes,buf,sizeof(mes));

    char str[BUFSIZ];
    sprintf(str,"select request from group_member where (gid=%d and mid=%d)",mes.recver,mes.sender);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        Send_pack(fd,VIEW_GROUP_RECORD_APPLY,"你还不是群成员，请先添加群");
        printf("[%d]查询群聊天记录失败\n",mes.sender);

         Send_pack(fd,VIEW_GROUP_RECORD_APPLY,"over");
        return ;
    }

    if(atoi(row[0])==2)
    {
        Send_pack(fd,VIEW_GROUP_RECORD_APPLY,"你还不是群成员,请等待管理审核");
        printf("[%d]查询群聊天记录失败\n",mes.sender);

         Send_pack(fd,VIEW_GROUP_RECORD_APPLY,"over");
        return ;
    }

    //获得群成员的id
    memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where (gid=%d and mid!=%d)",mes.recver,mes.sender);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result1;  
    MYSQL_ROW  row1;
    result1=mysql_store_result(&mysql);
    row1=mysql_fetch_row(result1);

    char str1[BUFSIZ];
    memset(str1,0,sizeof(str1));
    sprintf(str1,"select send_name,message from group_message where (recv_name=%d and gid=%d) || (send_name=%d and recv_name=%d)",mes.sender,mes.recver,mes.sender,atoi(row1[0]));
    printf("str1:%s\n",str1);
    ret=mysql_real_query(&mysql,str1,strlen(str1));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result2;
    MYSQL_ROW  row2;
    result2=mysql_store_result(&mysql);
    
    int send_fd=Get_connfd(mes.sender);
    printf(">>>>>>\n");
    //循环读取每一行数据
    while((row2=mysql_fetch_row(result2)))
    {
        char str1[256];
        memset(str1,0,sizeof(str1));
        printf("1111111\n");
        sprintf(str1,"[%d]在群[%d]:%s",atoi(row2[0]),mes.recver,row2[1]);
        Send_pack(fd,VIEW_GROUP_RECORD_APPLY,str1);
    }

    Send_pack(fd,VIEW_GROUP_RECORD_APPLY,"over");
    printf("[%d]查询了群[%d]的聊天记录\n",mes.sender,mes.recver);
}

void Set_group_admin(int fd,char* buf)
{
    Group_leader leader;
    memcpy(&leader,buf,sizeof(leader));
    printf("server set flag:%d\n",leader.flag);
    printf("server set sender:%d\n",leader.sender);
    printf("server set recver:%d\n",leader.recver);
    printf("server set admin:%d\n",leader.admin);
    printf("server set message:%s\n",leader.message);

    //查询操作者的地位
    char str[BUFSIZ];
    sprintf(str,"select status from group_member where (mid=%d and gid=%d)",leader.sender,leader.recver);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    int send_fd=Get_connfd(leader.sender);

    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {
        printf("没有群[%d],操作无效\n",leader.recver);
        char mes[256];
        sprintf(mes,"没有该群[%d],无法操作",leader.recver);
        Send_pack(send_fd,SET_GROUP_ADMIN_APPLY,mes);
        return ;
    }
    if(atoi(row[0])!=1)
    {
        printf("[%d]不是群主,操作无效\n",leader.sender);
        char mes[256];
        memset(&mes,0,sizeof(mes));
        sprintf(mes,"不是该群[%d]的群主，无法操作",leader.recver);
        Send_pack(send_fd,SET_GROUP_ADMIN_APPLY,mes);
        return ;
    }


    //再查询成员是否存在
    memset(str,0,sizeof(str));
    sprintf(str,"select status from group_member where (mid=%d and gid=%d)",leader.admin,leader.recver);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result1;  
    MYSQL_ROW  row1;
    result1=mysql_store_result(&mysql);
    row1=mysql_fetch_row(result1);
    if(row1==NULL)
    {

        printf("[%d]不是群成员,操作无效\n",leader.admin);
        char mes[256];
        sprintf(mes,"[%d]不是该群[%d]成员，无法操作",leader.admin,leader.recver);
        Send_pack(send_fd,SET_GROUP_ADMIN_APPLY,mes);
        return ;
    }
    if(atoi(row1[0])==2)
    {
        printf("[%d]已经是群管理\n",leader.admin);
        char mes[256];
        sprintf(mes,"[%d]已经是该群[%d]管理",leader.admin,leader.recver);
        Send_pack(send_fd,SET_GROUP_ADMIN_APPLY,mes);
        return ;
    }




    char buff[BUFSIZ]; 
    sprintf(buff,"update group_member set status=2 where (gid=%d and mid=%d)",leader.recver,leader.admin);
    printf("buff:%s\n",buff);
    ret=mysql_real_query(&mysql,buff,strlen(buff));
    if(ret)
    {
        Mysql_with_error(&mysql);
        printf("[%d]设置管理员[%d]失败\n",leader.sender,leader.admin);
    }
    printf("[%d]设置管理员[%d]成功\n",leader.sender,leader.admin);

    
    char mes[256];
    sprintf(mes,"您已经成功设置[%d]为管理员",leader.admin);
    Send_pack(send_fd,SET_GROUP_ADMIN_APPLY,mes);


    int recv_fd=Get_connfd(leader.admin);

    if(recv_fd<0)
    {
        char mes[256];
        sprintf(mes,"[%d]不在线",leader.admin);
        Send_pack(send_fd,SET_GROUP_ADMIN_APPLY,mes);
        return;
    }
    memset(&mes,0,sizeof(mes));
    sprintf(mes,"您已经成为该群[%d]的管理员",leader.recver);
    Send_pack(recv_fd,SET_GROUP_ADMIN_APPLY,mes);
}
void Kick(int fd,char* buf)
{
    Group_leader leader;
    memcpy(&leader,buf,sizeof(leader));
    printf("server kick flag:%d\n",leader.flag);
    printf("server kick sender:%d\n",leader.sender);
    printf("server kick recver:%d\n",leader.recver);
    printf("server kick admin:%d\n",leader.admin);
    printf("server kick message:%s\n",leader.message);


    //群是否存在
    char str[BUFSIZ];
    sprintf(str,"select status from group_member where (mid=%d and gid=%d)",leader.sender,leader.recver);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    int send_fd=Get_connfd(leader.sender);
    MYSQL_RES *result1;  
    MYSQL_ROW  row1;
    result1=mysql_store_result(&mysql);
    row1=mysql_fetch_row(result1);
    if(row1==NULL)
    {
        printf("没有群[%d],操作无效\n",leader.recver);
        char mes[256];
        sprintf(mes,"没有该群[%d],无法操作",leader.recver);
        Send_pack(send_fd,KICK_APPLY,mes);
        return ;
    }
    if(atoi(row1[0])==3)
    {
        printf("[%d]不是群管理或群主,操作无效\n",leader.sender);
        char mes[256];
        sprintf(mes,"您不是该群[%d]群管理或群主，无法操作",leader.recver);
        Send_pack(send_fd,KICK_APPLY,mes);
        return ;
    }
    int send_status=atoi(row1[0]);
    //再查询成员是否存在
    memset(str,0,sizeof(str));
    sprintf(str,"select status from group_member where (mid=%d and gid=%d)",leader.admin,leader.recver);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(row==NULL)
    {

        printf("[%d]不是群成员,操作无效\n",leader.admin);
        char mes[256];
        sprintf(mes,"[%d]不是该群[%d]成员，无法操作",leader.admin,leader.recver);
        Send_pack(send_fd,KICK_APPLY,mes);
        return ;
    }
    int admin_status=atoi(row[0]);
    int recv_fd=Get_connfd(leader.admin);//获得被踢成员的端口号
    //管理员地位为1或2
    //如果踢出成员地位为3
    if(send_status<admin_status)
    {
        char buff[BUFSIZ];
        sprintf(buff,"delete from group_member where gid=%d and mid=%d",leader.recver,leader.admin);
        printf("buff:%s\n",buff);
        ret=mysql_real_query(&mysql,buff,strlen(buff));
        if(ret)
        {
            Mysql_with_error(&mysql);
            printf("[%d]踢出[%d]失败\n",leader.sender,leader.admin);
        }
        printf("[%d]踢出[%d]成功\n",leader.sender,leader.admin);
        char mes[256];
        sprintf(mes,"您踢出群成员[%d]成功",leader.admin);
        Send_pack(send_fd,KICK_APPLY,mes);


        if(recv_fd<0)
        {
            char mes[256];
            sprintf(mes,"[%d]不在线",leader.admin);
            Send_pack(send_fd,KICK_APPLY,mes);
            return ;
        }

        memset(&mes,0,sizeof(mes));
        sprintf(mes,"您已经被管理员[%d]踢出群[%d]",leader.sender,leader.recver);
        Send_pack(recv_fd,KICK_APPLY,mes);
        return ;
    }
    else 
    {
        printf("操作者[%d]的群[%d]权限不够\n",leader.sender,leader.recver);
        char mes[256];
        sprintf(mes,"您在该群[%d]的权限不够",leader.recver);
        Send_pack(send_fd,KICK_APPLY,mes);
        return ;
    }


    /*//先查询他是不是管理员
   memset(str,0,sizeof(str));
    sprintf(str,"select mid from group_member where gid=%d and (status=1 ||status=2)",leader.recver);
    printf("str:%s\n",str);
    ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }
    MYSQL_RES *result;  
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    if(leader.sender==atoi(row[0]))
    {
        printf("[%d]不是群管理或群主,操作无效\n",leader.sender);
        char mes[256];
        sprintf(mes,"您不是该群[%d]群管理或群主，无法操作",leader.recver);
        Send_pack(send_fd,KICK_APPLY,mes);
    }*/

}
int Check_relationship2(int fd,int send,int recv)
{
    char str[BUFSIZ];
    sprintf(str,"select status from friend where (uid=%d and fid=%d and request=2) or (uid=%d and fid=%d and request=2)",send,recv,recv,send);
    printf("str:%s\n",str);
    int ret=mysql_real_query(&mysql,str,strlen(str));
    if(ret)
    {
        Mysql_with_error(&mysql);
    }

    MYSQL_RES *result; 
    MYSQL_ROW  row;
    result=mysql_store_result(&mysql);
    row=mysql_fetch_row(result);
    printf("here\n");
    if(row==NULL)
    {
        /*printf("why>\n");
        char mes[256];
        sprintf(mes,"[%d]还不是您的好友",recv);
        Send_pack(fd,RECV_FILE,mes);*/
        //Send_pack(fd,PRIVATE_CHAT_APPLY,"0");
        return -1;
    }
    if(atoi(row[0])==2)
    {
        char mes[256];
        sprintf(mes,"[%d]已经被屏蔽",recv);
        Send_pack(fd,RECV_FILE,mes);
        return 0;
    }
    return 1;
}

void Upload(int fd,char* buf)
{
    file_t file;
    memcpy(&file,buf,sizeof(file));
    printf("file server flag:%d\n",file.flag);
    printf("file server send:%d\n",file.sender);
    printf("file server recv:%d\n",file.recver);
    printf("file server name:%s\n",file.file_name);
    //printf("file server message:%s\n",file.message);
    printf("file server file_size:%d\n",file.file_size);



    char buff[1024];
    memset(buff,0,sizeof(buff));
    char file_name[100];
    memset(file_name,0,sizeof(file_name));
  

    strcpy(file_name,file.file_name);
    memset(buff,0,sizeof(buff));



    //FILE * fp=fopen(file_name,"w+");
    //fclose(fp);
    int fp = open(file_name,O_CREAT |O_TRUNC ,0666);
    if(fp < 0)
    {
        perror("open fail");
    }
    close(fp);

    fp = open(file_name,O_APPEND | O_WRONLY);
    if(fp < 0)
        perror("open fail");
    memset(buff,0,sizeof(buff));

    int len=0;
    int write_len;
    int i=0;
    //接收文件
     while((len=recv(fd,buff,sizeof(buff),0))>0)
    {
        
        i++;
        printf("server recv len:%d\n",len);

        if(strcmp(buff,"end") == 0)
            break;
        if(len<0)
        {
            printf("recv file fail\n");
            return ;
        }

        
        write_len=write(fp,buff,len);
   
        memset(buff,0,sizeof(buff));
    }
    close(fp);
    printf("接受文件成功！\n");



    //int fd=Get_connfd(file.sender);

   int flag=Check_relationship2(fd,file.sender,file.recver);

    if(flag>0)
    {
        char str[BUFSIZ];
        memset(str,0,sizeof(str));
        //用户  朋友  文件名  请求为0 未发送
        sprintf(str,"insert into file (uid,fid,filename,request)values('%d','%d','%s','%d')",file.sender,file.recver,file.file_name,0);
        printf("str:%s\n",str);
        int ret=mysql_real_query(&mysql,str,strlen(str));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }


        int recv_fd=Get_connfd(file.recver);
        if(recv_fd<0)
        {
            char mes[256];
            memset(&mes,0,sizeof(mes));
            sprintf(mes,"[%d]不在线",file.recver);
            Send_pack(fd,RECV_APPLY,mes);

            //free(buf);
            return ;
        }


        char mes[256];
        memset(&mes,0,sizeof(mes));
        sprintf(mes,"[%d]向你发送了一个文件:%s到了服务器,请去下载",file.sender,file.file_name);
        Send_pack(recv_fd,RECV_APPLY,mes);


        char str1[BUFSIZ];
        memset(str1,0,sizeof(str1));
        //用户  朋友  文件名  请求为1 已发送
        sprintf(str1,"update file set request=1 where (uid=%d and fid=%d)",file.sender,file.recver);
        printf("str1:%s\n",str1);
        ret=mysql_real_query(&mysql,str1,strlen(str1));
        if(ret)
        {
            Mysql_with_error(&mysql);
        }
    }
    else if(flag==0)
    {
        printf("[%d]和[%d]处于屏蔽状态\n",file.sender,file.recver);
        char mes1[256];
        memset(&mes1,0,sizeof(mes1));
        sprintf(mes1,"你与[%d]处于屏蔽状态,无法发送文件",file.recver);
        Send_pack(fd,RECV_APPLY,mes1);
        free(buf);
        return ;
    }
    else if(flag<0)
    {
        printf("why>\n");
        char mes[256];
        sprintf(mes,"[%d]还不是您的好友",file.recver);
        Send_pack(fd,RECV_APPLY,mes);
        return ;
    }
}

/*void* Send_file(void *arg)
{
    arg=(char*)arg;
    file_t file;
    memcpy(&file,arg,sizeof(file));
    printf("flag= %d\n",file.flag);

    printf("server file_flag=%d\n",file.flag);
    printf("server file_name=%s\n",file.file_name);
    printf("server file_recver=%d\n",file.recver);
    printf("server file_sender=%d\n",file.sender);
    printf("server file_size=%d\n",file.file_size);*/
    //printf("server file_pos=%lld\n",file.pos);


    // //得到好友套接
  //  int recv_fd=Get_connfd(file.recver);

//    printf("sercver file mes=%s\n",file.data);
    
    
   /* if(strcmp(file.data,"over")==0)
    {
        return NULL;
    }*/
    

    //向好友转发
  //  send(recv_fd,arg,1024,0);
  //  free(arg);
  //  return NULL;
   /* PACK request;
    memcpy(&request,buf,sizeof(request));



	char filename[50];
	memset(filename, 0, sizeof(filename));

	printf("收到文件名:%s\n",request.data);
    strcpy(filename,request.data);
	usleep(100000);*/

  


    /*char data[10];
    memset(data,0,sizeof(data));
    sprintf(data,"start");
    //发送下就绪信息
    if(Send_file_pack(fd,0,sizeof(data),data)<0)
    {
        my_err("write error",__LINE__);
    }*/
	 
//}

void Download(int fd,char* buf)
{
    file_t file;
    memcpy(&file,buf,sizeof(file));
    printf("server recv file name=%s",file.file_name);


    file_t file_t;
    file_t.flag=RECV_FILE;
    strcpy(file_t.file_name,file.file_name);

    char buff[BUFMAX];
    memcpy(buff,&file_t,sizeof(file_t));

    send(fd,buff,sizeof(buff),0);


    char buffer[1024];
    memset(buffer,0,sizeof(buffer));

    int fp=open(file_t.file_name,O_RDONLY);
    printf("file name=%s\n",file_t.file_name);

    if(fp<0)
    {
        printf("open fail\n");
        char mes[256];
        sprintf(mes,"文件%s不存在",file_t.file_name);
        Send_pack(fd,RECV_APPLY,mes);
        return ;
    }

    else 
    {
        int i = 0;
        memset(buffer,0,sizeof(buffer));
        int len= 0;
        
        while((len=read(fp,buffer,sizeof(buffer)))>0)
        {
            i++;
            
            printf("len= %d\n",len);

            if(send(fd,buffer,len,0) < 0)
            {
                printf("Send file fail\n");
                break;
            }
            
            memset(buffer,0,sizeof(buffer));

        }
        
        close(fp);
        memset(buffer,0,sizeof(buffer));

        sleep(1);
        strcpy(buffer,"end");
        send(fd,buffer,strlen(buffer),0);
        
        printf("发送成功\n");
        remove(file.file_name);

        char mes[256];
        memset(mes,0,sizeof(mes));
        sprintf(mes,"账户 :%d 接收文件%s成功",fd,file.file_name);
        Send_pack(fd,RECV_APPLY,mes);
    }

}

void Connect_mysql()
{
    mysql_init(&mysql);
    //初始化数据库
    mysql_library_init(0,NULL,NULL);
    if(!mysql_real_connect(&mysql,"47.94.14.45","Linux_7136","18861757136","chat_room_7136",0,NULL,0))
    {
        sys_err("connect error!",__LINE__);
    }
    if(mysql_set_character_set(&mysql,"utf8"))
    {
        sys_err("set error!",__LINE__);
    }
    printf("连接MYSQL数据库成功!\n");
}

void Close_mysql(MYSQL mysql)
{
    mysql_close(&mysql);
    //mysql_free_result(result);
    mysql_library_end();
    printf("MYSQL数据库关闭!\n");
}

void my_err(const char* err_string,int line)
{
    fprintf(stderr,"line:%d",line);
    perror(err_string);
    exit(1);
}
void Clear_buf()
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
char getch()
{
	char ch;

    system("stty -echo");//不回显
    system("stty -icanon");//设置一次性读完操作，如使用getchar()读操作,不需要按回车
    ch = getchar();
    system("stty icanon");//取消上面的设置
    system("stty echo");//回显

    return ch;
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
void Mysql_with_error(MYSQL* mysql)
{
    fprintf(stderr,"%s\n",mysql_error(mysql));
    mysql_close(mysql);
    return ;
}

void Send_pack(int fd,int flag,char* buf)
{
    char str[BUFSIZ];
    message mes;

    mes.flag=flag;
    strcpy(mes.message,buf);
    printf("server send message:%s\n",mes.message);
    memcpy(str,&mes,sizeof(mes));
    
    printf("server send message:%s\n",mes.message);
    printf("server send flag:%d\n",mes.flag);
    if(send(fd,str,sizeof(str),0)<0)
    {
        my_err("send error!",__LINE__);
        close(fd);
    }
}

