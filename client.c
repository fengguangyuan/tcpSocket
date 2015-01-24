/***************************************************
* FILENAME：pthread_client.c
* DESCRIPTION：创建子线程来接收客户端的数据
* COPYRIGHT:SA14226417	Mr.feng
***************************************************/ 
#include "file_ops.h"    
#include <sys/types.h> 
#include <sys/socket.h> 
#include <stdio.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <string.h>
#include <pthread.h>    
#include <time.h>

#define MAX_SIZE 256

/*the main thread tcp socket_fd */
int sockfd; 

/*declarations of functions used in this file*/
struct fileInfo *getFileInfo();
void *recv_data(void *block);
void *write_data(void *block);

int main(int argc,char *argv[]) 
{ 
    struct sockaddr_in address;    
    struct thread_block block[THREAD_NUM]; //thread block info
    int len;                               //length of sockaddr_in

    pthread_t thd[THREAD_NUM] = {0};       //array of threads
    int result;                            //result of connection
    int i;                                 //var of circyling
    char char_send[MAX_SIZE] = {0};        //used to send init info

    //malloc and initialize a new mem buf for the public buffer->buf_1
    buf_1 = malloc(sizeof(char)*BLOCK_SIZE*times);
    memset(buf_1,0,BLOCK_SIZE*times);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1)
    { 
         perror("socket"); 
         exit(EXIT_FAILURE); 
    } 
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){ 
         perror("sock"); 
         exit(1); 
    } 
    bzero(&address,sizeof(address)); 
    address.sin_family = AF_INET; 
    address.sin_port = htons(atoi("8888")); 
    inet_pton(AF_INET,"127.0.0.1",&address.sin_addr); 
    len = sizeof(address); 
    
    if((result = connect(sockfd, (struct sockaddr *)&address, len)) == -1) 
    { 
          perror("connect"); 
          exit(EXIT_FAILURE); 
    } 
    printf("CLIENT : connect success!!!\n");
    recv(sockfd,char_send,MAX_SIZE,0);
    printf("SERVER : %s\n",char_send);

    //send the file name to the server.
    struct fileInfo *file = getFileInfo();
    char filename[256] = {0};
    strcat(filename,file->filename);

    //create threads
    for(i = 0; i < THREAD_NUM; i++)
    { 
        block[i].filename = filename;
        printf("block-> file name is %s\n",file->filename); 
	block[i].start = 0;
	block[i].end = file->filesize;
	block[i].index = i;

	//the receiving data threads
        if(pthread_create(&thd[i], NULL, recv_data, &block[i]) !=0 )
	{ 
            perror("pthread_create failed!!\n"); 
            break; 
      	}
        else printf("thread ceate successfully!!!\n"); 
        printf("************************************************\n");
     }//end for

     //create a new thread for writting data into a new file 
     if(pthread_create(&thd[i], NULL, write_data, &block[i-1]) !=0 )
     { 
         perror("pthread_create failed!!\n");      	      
     }
     else printf("WRITE thread ceate successfully!!!\n"); 
     printf("************************************************\n");
     close(sockfd); 
     free(file);
     pthread_exit(NULL);
     exit(0); 
}    

/**********************************************************
 * FUNCTION： recv_data
 * DESCRIPTION： receive the data from the server
 * ARGUMENTS：
 *	arg: accept a pointer of thread_block
 * RETURN： void *
 *********************************************************/ 
 void *recv_data(void *arg) 
 { 
    struct thread_block *block =(struct thread_block*)arg;
    struct Data_Block *data;
    struct sockaddr_in address;    
    int sockfd; 
    int len,pack_len = 0;
    int tmp = 0,i;
    int result; 
    char char_recv[BLOCK_SIZE] = { 0 };
    long byteswrite = 0;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1) 
    { 
        perror("socket"); 
        exit(EXIT_FAILURE); 
    } 
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){ 
        perror("sock"); 
        exit(1); 
    } 
    bzero(&address,sizeof(address)); 
    address.sin_family = AF_INET; 
    address.sin_port = htons(atoi("8888")); 
    inet_pton(AF_INET,"127.0.0.1",&address.sin_addr); 
    len = sizeof(address); 

    if((result = connect(sockfd, (struct sockaddr *)&address, len)) == -1) 
    { 
        perror("connect"); 
        exit(EXIT_FAILURE); 
    } 
    //receive the confirm infomation
    while(recv(sockfd,char_recv,MAX_SIZE,0) == -1)
       recv(sockfd,char_recv,MAX_SIZE,0);
    printf("SERVER : %s\n",char_recv);
    memset(char_recv,0,BLOCK_SIZE);

    printf("Thread %d start!!!!!!!!!!!!!!!!\n",block->index);
    printf("Reading file name %s\n",block->filename);
    printf("Reading file size %ld\n",block->end - block->start);
    while( (block->start + byteswrite) < block->end)//receive messages from one client untill rec the sign of "exit"
    { 
           //pthread_mutex_lock(&client_mymutex_thread);
           
           //this is a different things that we should decalaration that makes things better
           pthread_mutex_lock(&mymutex);
           while(-1 == (pack_len = recv(sockfd,&data,sizeof(data),0)))
           {
              pack_len = recv(sockfd,&data,sizeof(data),0);
           }
           if(block->end - byteswrite < BLOCK_SIZE)
           {
               
               while(-1 == (len = recv(sockfd,char_recv,block->end - byteswrite,0)))
                   len = recv(sockfd,char_recv,block->end - byteswrite,0);
           }
           else
           {
               while((len = recv(sockfd,char_recv,BLOCK_SIZE,0)) < BLOCK_SIZE)
               {
                   len += (tmp = recv(sockfd,char_recv+len,BLOCK_SIZE - len,0));
                   if(len == BLOCK_SIZE || tmp == 0)
                     break;
               }
           }           
           pthread_mutex_unlock(&mymutex);

           //start to put data into the buf_1...
           while(1)
           {
               if((tmp = myrecv(sockfd,buf_1,char_recv,len)) != 0)
               {    
                   break;
               }
           }
           memset(char_recv,0,BLOCK_SIZE);

           //pthread_mutex_unlock(&client_mymutex_thread);

           byteswrite += len;
           tmp = 0;
    } 
    //if there still has data in the buf_1,wake up the write_data()
    if(bufCnt != 0)
    {
        FILEWRITE = 1;
        pthread_cond_signal(&cond_1);
    }
    close(sockfd);
    //exit this thread of a client
    pthread_exit(NULL); 
} 

/**********************************************************
 * FUNCTION： write_data
 * DESCRIPTION： write the data into a local file
 * ARGUMENTS：
 *	arg: accept a pointer of thread_block
 * RETURN： void *
 *********************************************************/ 
void *write_data (void *arg)
{
        struct thread_block *block = (struct thread_block *)arg;
        char *buf_2 = malloc(sizeof(char)*BLOCK_SIZE);
//        char *buf_2[BLOCK_SIZE] = {0};
        FILE* fp;
        int tmp = 0;
        memset(buf_2,0,BLOCK_SIZE);
	if((fp = fopen(block->filename,"wb")) == NULL)
            perror("Thread file open failed.....\n");

        pthread_mutex_lock(&client_mymutex_write);
        while(1)
        {
            while(FILEWRITE == 0)
            {    
                //lock this thread,when these is no data to read from the buf
                pthread_cond_wait(&cond_1,&client_mymutex_write);            
                break;
            }

            if((tmp = block->end - ftell(fp)) < BLOCK_SIZE)
                tmp =  myfwrite(buf_2,sizeof(char),tmp,fp);                       
            else
                tmp =  myfwrite(buf_2,sizeof(char),BLOCK_SIZE,fp);                 
            if(ftell(fp) == block->end)
            {
                pthread_mutex_unlock(&client_mymutex_write);
                break;
            }
            pthread_mutex_unlock(&client_mymutex_write);
        }

        fclose(fp);
        free(buf_2);
        free(buf_1);
        pthread_exit(NULL);
}


/**********************************************************
 * FUNCTION： getFileInfo
 * DESCRIPTION： get the target file infomation
 * ARGUMENTS：
 *	NULL
 * RETURN： struct fileInfo
 *********************************************************/ 
struct fileInfo *getFileInfo()
{
	char filename[256] = {0};
        char tem[256] = "new_\0";
	struct fileInfo *file = malloc(sizeof(struct fileInfo));
        size_t filesize = 0;
	printf("please send the file name you want to get!\n");
        scanf("%s",filename);
        printf("the file name is :\t%s\n",filename);
	while(-1 == send(sockfd,filename,strlen(filename),0))
	{
		perror("file name send failed!! Resending.......\n");
	}
        printf("等待接收服务器文件信息，请稍后。。。\n");
	while(-1 == recv(sockfd,&filesize,sizeof(size_t),0))
	{
           perror("文件信息接收失败，重新接收中。。。。\n");
	}
	printf("文件长度已找到(%d)，即将进入传送阶段，请稍后。。。\n",filesize);
        sleep(1);
        strcat(tem,filename);
        file->filename = tem;
        file->filesize = filesize > 0?filesize:0;
	return file;
}
