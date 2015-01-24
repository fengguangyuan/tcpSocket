/*****************************************************************
 * FILENAME:file_ops.h
 *
 * DESCRIPTION:provider interfaces or functions for the c/s
 *
 * COPYRIGHT:SA14226417	Mr.feng
 *****************************************************************/

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

#define THREAD_NUM 1         //thread num
#define BLOCK_SIZE 1024*8   //read data size by one time

/**************************************************************/
/*times means the size of buf_1,then the others 
/*control reading or writting the buffer
/**************************************************************/
char *buf_1;                // the public buffer for temp data
int times = 100;            //the times of BLCOK_SIZE
char *server_buf_1;         //BUFFER ONE    
char *server_buf_2;         //BUFFER TWO
int writeCnt = 0,readCnt = 0,bufCnt = 0;

/*flags will be used to call the threads up*/ 
int FILEREAD = 0,FILEWRITE = 0;

//#define printf //

//condition mutex for the buf-reading thread in client
pthread_cond_t cond_1=PTHREAD_COND_INITIALIZER;

/*other mutexes used in the diff threads*/
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mymutex_read = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mymutex_write = PTHREAD_MUTEX_INITIALIZER;

/*mutexes fro users*/
pthread_mutex_t client_mymutex_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mymutex_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_mymutex_send = PTHREAD_MUTEX_INITIALIZER;

/*mutexes for buffer*/
pthread_mutex_t buf_mymutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buf_read = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buf_write = PTHREAD_MUTEX_INITIALIZER;

//semaphore,to be used int the future
sem_t sem;//信号量 

/*client functions*/
int myfwrite(char *buf,int size,int length,FILE *fp);
int myrecv(int sock_fd, char *buf,const char *src_buf, int size);

/*server functions*/
int myfread(char *,const char *,int);

/*public funtions*/
long int getFileSize(const char *fileName);
int bufIsAvailable();
int bufIsEmpty();
int bufIsFul();

//the infomation block of one thread
struct thread_block
{
    char *filename;
    int *sockfd;
    int index;
    long start;
    long end;
};
/**/
struct Data_Block
{
    long start_pos;
};
/*file info block*/
struct fileInfo
{
    char *filename;
    long filesize;
};

//////////////////////////////////////////////////////////////////////////////
/*implementation of all interfaces*/
//////////////////////////////////////////////////////////////////////////////
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
/*****************************************************/
/**FUNCTION: bufIsFul
/**DESCRIPTION: if the buf_1 is full,then return 1,or 0
/**RETURN: 1 or 0
/*****************************************************/
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

/******************************************************************/
/*FUNCTION: myrecv
/*DESCRIPTION: store the socket-recved data into the public buffer
/*ARGUMENTS:
/*	sock_fd: tcp socket handler
/*	src_buf: the temp buff used to save the data from sock-recv
/*	size: the size of buf
/******************************************************************/
int myrecv(int sock_fd, char *buf,const char *src_buf, int size)
{
    char *tmp;
    pthread_mutex_lock(&buf_mymutex);
    if(bufIsAvailable())
    {   
       tmp = memcpy(buf+BLOCK_SIZE * writeCnt,src_buf,size);
       writeCnt++;       
       if(writeCnt == times)
           writeCnt = 0;
       bufCnt++;
       if(bufCnt == times / 2)
       {
           FILEWRITE = 1;
           pthread_cond_signal(&cond_1);
       }

    }
    else if(bufIsFul())
    {
        pthread_mutex_unlock(&buf_mymutex);
        return 0;
    }
    pthread_mutex_unlock(&buf_mymutex);
    return size;
}
/*****************************************************************/
/*FUNCTION: myfwrite
/*DESCRIPTION: client write data into a file
/*ARGUMENTS:
/*	buf: writting buffer name
/*	size: the size of buf
/*	length: the size of data to write
/*	fp: file handler
/*****************************************************************/
int myfwrite(char *buf,int size,int length,FILE *fp)
{
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
        readCnt++;
        if(readCnt == times)
            readCnt = 0;
        bufCnt--;
        if(bufCnt == 0)
        {
            FILEWRITE = 0;
        }
    }
    pthread_mutex_unlock(&buf_mymutex);
    
    //write data into file
    length = fwrite(buf,sizeof(char),length,fp);

    //clear the buffer
    memset(buf,0,BLOCK_SIZE);
    return length;
}

/*********************************************************************/
/*expand functions :
/*server read data from a file,but it's verifying....
/*********************************************************************/
//fetch data from buffer-2
int myfread(char *buf,const char *src_buf,int length)
{
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
        memcpy(buf+BLOCK_SIZE*writeCnt,src_buf,length);
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
    //memset(buf,0,sizeof(buf));
    printf("write data bytes is %d\n",length);
    return length;
}
