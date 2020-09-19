#include "pthreadpool.h"

void pool_init(int max_thread_num)  
{  
    
    pool=(threadpool_t *)malloc(sizeof(threadpool_t));  
  
    pthread_mutex_init(&(pool->lock),NULL);  
    pthread_cond_init(&(pool->cond),NULL);  
  
    pool->queue_head=NULL;  
  
    pool->max_thread_num=max_thread_num;  
    pool->queue_size=0;  
  
    pool->shutdown=0;  
  
    pool->threads=(pthread_t *)malloc(max_thread_num*sizeof(pthread_t));  
    int i=0;  
    for(i=0;i<max_thread_num;i++)  
    {   
        pthread_create(&(pool->threads[i]),NULL,thread_routine,NULL);  
    }  
}  
  
  
  
//向线程池中加入任务  
int threadpool_add(void *(*process)(void *arg),void *arg)  
{  
    //构造一个新任务  
    threadpool_task *newworker=(threadpool_task *)malloc(sizeof(threadpool_task));  
    newworker->process=process;  
    newworker->arg=arg;  
    //置为空
    newworker->next=NULL;
  
    pthread_mutex_lock(&(pool->lock));  
    //将任务加入到等待队列中  
    threadpool_task *member=pool->queue_head;  
    if(member!=NULL)  
    {  
        while(member->next!=NULL)  
            member=member->next;  
        member->next=newworker;  
    }  
    else  
    {  
        pool->queue_head=newworker;  
    }  
  
    assert(pool->queue_head!=NULL);  
  
    pool->queue_size++;  
    pthread_mutex_unlock (&(pool->lock));

    /*等待队列中有任务了，唤醒一个等待线程； 
    如果所有线程都在忙碌，这句没有任何作用*/  
    pthread_cond_signal(&(pool->cond));  
    return 0;  
}  
  
  
  
/*销毁线程池，等待队列中的任务不会再被执行，正在运行的线程会一直 
把任务运行完后再退出*/  
int threadpool_destroy()  
{  
    if(pool->shutdown)  
        return -1;//防止两次调用  
    pool->shutdown=1;  
  
    //唤醒所有等待线程，线程池要销毁了  
    pthread_cond_broadcast(&(pool->cond));  
  
    //阻塞等待线程退出，否则就成僵尸了  
    int i;  
    for(i=0;i<pool->max_thread_num;i++)  
        pthread_join(pool->threads[i],NULL);  
    free(pool->threads);  
  
    //销毁等待队列  
    threadpool_task *head=NULL;  
    while(pool->queue_head!=NULL)  
    {  
        head=pool->queue_head;  
        pool->queue_head=pool->queue_head->next;  
        free(head);  
    }  
    //条件变量和互斥量销毁  
    pthread_mutex_destroy(&(pool->lock));  
    pthread_cond_destroy(&(pool->cond));  
      
    free(pool);  
    //指针置空  
    pool=NULL;  
    return 0;  
}  
  
  
  
void *thread_routine(void *arg)  
{  
    //printf("starting thread 0x%ld\n",pthread_self());  
    while (1)  
    {  
        pthread_mutex_lock(&(pool->lock));  
        /*如果等待队列为0并且不销毁线程池，则处于阻塞状态; 
        pthread_cond_wait是一个原子操作，等待前会解锁，唤醒后会加锁*/  
        while (pool->queue_size==0&&!pool->shutdown)  
        {  
            //printf("thread 0x%ld is waiting\n",pthread_self());  
            pthread_cond_wait(&(pool->cond), &(pool->lock));  
        }  
  
        //线程池要销毁  
        if (pool->shutdown)  
        {  
            //遇到break,continue,return等跳转语句，不要忘记先解锁  
            pthread_mutex_unlock(&(pool->lock));  
            //printf ("thread 0x%ld will exit\n",pthread_self());  
            pthread_exit(NULL);  
        }  
  
       // printf("thread 0x%ld is starting to work\n",pthread_self());  
  
  
        assert(pool->queue_size!= 0);  
        assert(pool->queue_head!=NULL);  
          
        //等待队列长度减去1，并取出链表中的头元素  
        pool->queue_size--;  
        threadpool_task *worker=pool->queue_head;  
        pool->queue_head=worker->next;  
        pthread_mutex_unlock(&(pool->lock));  
  
        //调用回调函数，执行任务  
        (*(worker->process))(worker->arg);  
        free(worker);  
        worker=NULL;  
    }  
    //这一句是不可达的  
    pthread_exit(NULL);  
}  


/*
//测试
void *myfunc(void* arg)
{  
    printf("threadid is 0x%ld,working on task %d\n",pthread_self(),*(int *)arg);  
    
    //休息一秒，延长任务的执行时间 
    sleep(1); 
    return NULL; 
}  
  
int main(int argc,char **argv)  
{  
    //static threadpool_t *pool;
    //线程池中最多三个活动线程 
    pool_init (3);
    
  
      
    //连续向池中投入10个任务
    int *workingnum=(int *)malloc(sizeof(int)*10);  
    int i;  
    for(i=0;i<10;i++)  
    {  
        workingnum[i]=i;  
        threadpool_add(myfunc,&workingnum[i]);  
    }  
    //等待所有任务完成  
    sleep (5);  
    //销毁线程池  
    pool_destroy();  
    free(workingnum);  
    return 0;  
}*/

