#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/socket.h> 
#include <stdio.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <string.h>
#include <semaphore.h>

#define THREAD_NUM 1
#define BLOCK_SIZE 1024*1
//times means the size of buf_1,then the others 
//control reading or writting the buffer
int times = 10,writeCnt = 0,readCnt = 0,bufCnt = 0;
int writeCnt_2 = 0,readCnt_2 = 0,bufCnt_2 = 0;
char *buf_1;  // the public buffer for temp data
char *server_buf_1;
char *server_buf_2;
//int buf_isFul=0,buf_isAailable = 1;
//flags will be used 
int FILEREAD = 0,FILEWRITE = 0;

//#define printf //

//mutex
pthread_cond_t cond_1=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_2=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mymutex_read = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mymutex_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mymutex_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mymutex_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_mymutex_send = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buf_mymutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buf_read = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buf_write = PTHREAD_MUTEX_INITIALIZER;
//sem
sem_t sem;//信号量 
/*client*/
int myfwrite(char *buf,int size,int length,FILE *fp);
int myrecv(int sock_fd, char *buf,const char *src_buf, int size);
/*server*/
int myfread(char *,const char *,int);
long int getFileSize(const char *fileName);
int bufIsAvailable();
int bufIsEmpty();
int bufIsFul();

//the info of one thread block
struct thread_block
{
    char *filename;
    int *sockfd;
    int index;
    long start;
    long end;
};
//
struct Data_Block
{
    long start_pos;
};

struct fileInfo
{
    char *filename;
    long filesize;
};
////////////////////////////////////////////////////////
long int getFileSize(const char *fileName)
{
	struct stat buf;
	int result = stat(fileName,&buf);
	if(result != 0)
		perror("File reading failed!!Please confirm it exists or not!\n");
	else
	{
		printf("locate file successfully!Return the size.\n");
		return buf.st_size;
	}
}
/*
**/
int bufIsFul()
{
    return bufCnt == times ? 1 : 0;
}
/*
**/
int bufIsEmpty()
{
    return bufCnt == 0? 1 : 0;
}
/*
**/
int bufIsAvailable()
{
    return bufCnt != times  ? 1 : 0;
}
/*
**/
int myrecv(int sock_fd, char *buf,const char *src_buf, int size)
{
    char *tmp;
    pthread_mutex_lock(&buf_mymutex);
    if(bufIsAvailable())
    {   
       printf("the data length in char_recv is %d\n",strlen(src_buf));
       tmp = memcpy(buf+BLOCK_SIZE * writeCnt,src_buf,size);
//       tmp = strcpy(buf+BLOCK_SIZE * writeCnt,src_buf);
//       tmp = strcpy(buf+BLOCK_SIZE * writeCnt,src_buf,size);
       if(tmp[0] == '\0')
           printf("%s\n",tmp);
       writeCnt++;       
       printf("recv=======================================%d\n",writeCnt);
       if(writeCnt == times)
           writeCnt = 0;
       bufCnt++;
       if(bufCnt == times / 2)
       {
           FILEWRITE = 1;
           pthread_cond_signal(&cond_1);
       }

       printf("buff count is %d\n",bufCnt);
       //buf_store_start = buf_front;
           printf("put data into the buf_1 successfully!!!!!\n");
           printf("******************************************************\n");
    }
    else if(bufIsFul())
    {
        pthread_mutex_unlock(&buf_mymutex);
        return 0;
    }
    pthread_mutex_unlock(&buf_mymutex);
    return size;
}
/*client write data into a file*/
int myfwrite(char *buf,int size,int length,FILE *fp)
{
//    printf("the buf_1's length is : %d\n",strlen(buf));
//    pthread_mutex_lock(&mymutex);
    char *tmp;
    pthread_mutex_lock(&buf_mymutex);
    if(bufIsEmpty())
    { 
       pthread_mutex_unlock(&buf_mymutex);
       return 0;
    }
    else
    {
        //put the data into the buffer by order
        memcpy(buf,buf_1+BLOCK_SIZE*readCnt,length);
        //strncpy(buf,buf_1+BLOCK_SIZE*readCnt,length);
        //if(strlen(buf) == 0)
        //    printf("content :1111111111111111111111111111111111111111111\n");
        readCnt++;
        printf("read============================================%d\n",readCnt);
        if(readCnt == times)
            readCnt = 0;
        bufCnt--;
        if(bufCnt == 0)
        {
            FILEWRITE = 0;
        }
        printf("buff count int fwrite is %d\n",bufCnt);
    }
    printf("write thread start to write..........%ld\n",ftell(fp));                 
    printf("write thread put data into file successfully!!!!!!!!\n");
    printf("write data bytes is %d\n",length);
    printf("write data bytes is %d\n",strlen(buf));
    pthread_mutex_unlock(&buf_mymutex);
    
    //write data into file
    length = fwrite(buf,sizeof(char),length,fp);
    memset(buf,0,BLOCK_SIZE);
    printf("write data bytes is %d\n",length);
    return length;
}
/*server read data from a file*/
//
int myfread(char *buf,const char *src_buf,int length)
{
//    printf("the buf_1's length is : %d\n",strlen(buf));
//    pthread_mutex_lock(&mymutex);
    char *tmp;
    pthread_mutex_lock(&buf_mymutex);
    printf("start to put data into the buf_1...\n");
    if(bufIsFul())
    { 
       pthread_mutex_unlock(&buf_mymutex);
       return 0;
    }
    else
    {
        //put the data into the buffer by order
        printf("2222222222222222222222222222222222222222222\n");
        memcpy(buf+BLOCK_SIZE*writeCnt,src_buf,length);
        //strncpy(buf,buf_1+BLOCK_SIZE*writeCnt,length);
        //if(strlen(buf) == 0)
        //    printf("content :1111111111111111111111111111111111111111111\n");
        writeCnt++;
        printf("write============================================%d\n",writeCnt);
        if(writeCnt == times)
            writeCnt = 0;
        bufCnt++;
        if(bufCnt == times / 2)
        {
            FILEREAD = 1;
//            pthread_cond_signal(&cond_1);
        }
        printf("buff count int fwrite is %d\n",bufCnt);
    }
    printf("write thread put data into file successfully!!!!!!!!\n");
    printf("write data bytes is %d\n",length);
    printf("write data bytes is %d\n",strlen(buf));
    pthread_cond_signal(&cond_1);

    pthread_mutex_unlock(&buf_mymutex);
    return length;
}
/*
**/
int mysend(char *buf,int length)
{
//    printf("the buf_1's length is : %d\n",strlen(buf));
//    pthread_mutex_lock(&mymutex);
    char *tmp;
    pthread_mutex_lock(&buf_mymutex);
    if(bufIsEmpty())
    { 
       pthread_mutex_unlock(&buf_mymutex);
       return 0;
    }
    else
    {
        //put the data into the buffer by order
        tmp = memcpy(buf,buf_1+BLOCK_SIZE*readCnt,length);
        printf("%s\n",tmp);
        readCnt++;
        printf("read============================================%d\n",readCnt);
        if(readCnt == times)
            readCnt = 0;
        bufCnt--;
        if(bufCnt == 0)
        {
            FILEREAD = 0;
        }
        printf("buff count int fwrite is %d\n",bufCnt);
    }
    pthread_mutex_unlock(&buf_mymutex);
//    memset(buf,0,sizeof(buf));
    printf("write data bytes is %d\n",length);
    return length;
}
