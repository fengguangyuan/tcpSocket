/***************************************************
* 文件名：pthread_client.c
* 文件描述：创建子线程来接收客户端的数据
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
#define MAX_SIZE 256

struct fileInfo *getFileInfo();
char buf[BLOCK_SIZE*10] = {0};

void *recv_data(void *block);
int sockfd; 
int main(int argc,char *argv[]) 
{ 
    int len; 
    struct sockaddr_in address;    
    int result;
    int i,byte; 
    char char_send[MAX_SIZE] = { 0 }; 
    struct thread_block block[THREAD_NUM];
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

        long block_size = file->filesize / THREAD_NUM;
	pthread_t thd[THREAD_NUM] = {0};
        //create threads
        for(i = 0; i < THREAD_NUM; i++)
        { 
		block[i].filename = filename;
                printf("block-> file name is %s\n",file->filename); 
		block[i].start = i*block_size;
		if(block_size > file->filesize - block[i].start)
			block[i].end = file->filesize - block[i].start;
		else 
			block[i].end = block[i].start + block_size;
		block[i].index = i;
		if(pthread_create(&thd[i], NULL, recv_data, &block[i]) !=0 )//创建子线程 
	        { 
        	       perror("pthread_create failed!!\n"); 
        	       break; 
      		}
                else printf("thread ceate successfully!!!\n"); 
                printf("************************************************\n");
         //   printf("MSG FORM SERVER : \n%s\n",char_send);
       //     printf("************************************************\n");
        } 
        close(sockfd); 
        free(file);
        pthread_exit(NULL);
        exit(0); 
    }     

/*****************************************
    * 函数名称：recv_data_one
    * 功能描述：接受客户端的数据
    * 参数列表：fd——连接套接字
    * 返回结果：void *
    *****************************************/ 
 void *recv_data(void *arg) 
 { 
    int sockfd; 
    int len,pack_len = 0;
    int tmp = 0; 
    struct sockaddr_in address;    
    int result; 
    int i,byte; 
    char char_recv[BLOCK_SIZE] = { 0 };
    long byteswrite = 0;
    struct thread_block *block =(struct thread_block*)arg;
    struct Data_Block data;
    data.start_pos = 0;

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
    printf("thread %d connects successfully!!!\n",block->index);

    while(recv(sockfd,char_recv,MAX_SIZE,0) == -1)
       recv(sockfd,char_recv,MAX_SIZE,0);


    printf("SERVER : %s\n",char_recv);
    memset(char_recv,0,BLOCK_SIZE);

    printf("Thread %d start!!!!!!!!!!!!!!!!\n",block->index);
    printf("Reading file name %s\n",block->filename);
    printf("Reading file size %ld\n",block->end - block->start);
 //   sleep(1);
    FILE* fp;
	  if((fp = fopen(block->filename,"wt")) == NULL)
            perror("Thread %d file open failed.....\n");
        printf("Thread %d open file %s successfully!!!!!!!!\n",block->index,block->filename);
	  fseek(fp,block->start,0);	    
	  while( (block->start + byteswrite) < block->end)//receive messages from one client untill rec the sign of "exit"
    //while(data.start_pos != block->end)
	  { 
          //this is a different things that we should decalaration that makes things better
          printf("recv data by the %dth thread in the client!!\n",block->index);
           
          pthread_mutex_lock(&mymutex);
          //while(-1 == (pack_len = recv(sockfd,&data,sizeof(data),0)))
          //{
              pack_len = recv(sockfd,&data,sizeof(data),0);
              printf("pack head size is %d \n",pack_len);
              printf("data start position is %ld\n",data.start_pos);
          //}
          pack_len = 0;
          if(block->end - data.start_pos <= BLOCK_SIZE)
          {
             len = recv(sockfd,char_recv,block->end - data.start_pos,0);
          }
          else
          {
             while((len = recv(sockfd,char_recv,BLOCK_SIZE,0)) < BLOCK_SIZE)
             {
                 printf("the pre data size is %d\n",len);
                 len += (tmp = recv(sockfd,char_recv+tmp,BLOCK_SIZE - len,0));
                 printf("the expected data size is %d\n",BLOCK_SIZE - len);
                 if(len == 1024 || tmp == 0)
                   break;
                 printf("the last data size is %d\n",tmp);
             }
          }		   
          pthread_mutex_unlock(&mymutex);

          byteswrite += len;
          tmp = 0;

	        pthread_mutex_lock(&mymutex);
          fseek(fp,data.start_pos,0);
	        tmp = fwrite(char_recv,sizeof(char),BLOCK_SIZE,fp);
	        pthread_mutex_unlock(&mymutex);

          //printf("receive bytes : %d\n",strlen(char_recv));
          memset(char_recv,0,BLOCK_SIZE);
          printf("write data into the file from %ld\n totally %d bytes",data.start_pos,len);
          printf("Expected data size is %d,real size is %d\n",BLOCK_SIZE,len);
          printf("total size is %ld\n",byteswrite);
          printf("-------------------------------------------------------\n");
          tmp = 0;
	  } 
    close(sockfd);
    //exit this thread of a client
    pthread_exit(NULL); 
 } 
//get the file information from the server
struct fileInfo *getFileInfo()
{
	char filename[256] = {0};
  char tem[256] = "new_\0";
	struct fileInfo *file = malloc(sizeof(struct fileInfo));
  size_t filesize = 0;
	printf("please send the file name you want to get!\n");
	//sprintf(filename,"%s",stdin);
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
      sleep(1);
	}
	printf("文件长度已找到(%d)，即将进入传送阶段，请稍后。。。\n",filesize);
  strcat(tem,filename);
  file->filename = tem;
  file->filesize = filesize > 0?filesize:0;
	return file;
}
