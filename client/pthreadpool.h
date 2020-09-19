#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<pthread.h>
#include<assert.h>

typedef struct threadpool_task_t  
{  
    //回调函数，任务运行时会调用此函数，也可声明成其它形式
    void *(*process) (void *arg);  
    void *arg;/*回调函数的参数*/  
    struct threadpool_task_t *next;  
  
}threadpool_task;  
 


//线程池结构  
typedef struct  
{  
    pthread_mutex_t lock;  
    pthread_cond_t cond;  
  
    //链表结构，线程池中所有等待任务  
   threadpool_task *queue_head;  
  
    //是否销毁线程池  
    int shutdown;  
    pthread_t *threads;  
    //线程池中允许的活动线程数目  
    int max_thread_num;  
    //当前等待队列的任务数目 
    int queue_size;  
  
}threadpool_t; 

threadpool_t *pool;

int threadpool_add(void *(*process)(void *arg),void *arg);  
int threadpool_destroy();
void *thread_routine(void *arg); 
void pool_init(int max_thread_num);  

void *myfunc(void* arg);

