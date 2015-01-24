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
#include <time.h>
#define MAX_SIZE 256
int FLAG = 0;

struct fileInfo *getFileInfo();
int count = 0,buf_1_count = 0,buf_2_count =0,offset=0;
void *recv_data(void *block);
void *write_data(void *block);
int sockfd; 

int main(int argc,char *argv[]) 
{ 
    int len; 
    struct sockaddr_in address;    
    int result;
    int i,byte; 
    char char_send[MAX_SIZE] = { 0 }; 
    struct thread_block block[THREAD_NUM];
    int res = -1;
    //malloc a new mem buf
    buf_1 = malloc(sizeof(char)*BLOCK_SIZE*times);
    memset(buf_1,0,BLOCK_SIZE*times);
    //initialize sem
    //初始化信号量,初始值为0  
    res = sem_init(&sem, 0, 1);  
    if(res == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    } 

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
		block[i].start = 0;
//		if(block_size > file->filesize - block[i].start)
//			block[i].end = file->filesize - block[i].start;
//		else 
		block[i].end = file->filesize;
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
//                sleep(1);
                if(pthread_create(&thd[i], NULL, write_data, &block[i-1]) !=0 )//创建子线程 
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
    int tmp = 0,buf_count = 0; 
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
//    FILE* fp;
//    if((fp = fopen(block->filename,"wb")) == NULL)
//        perror("Thread file open failed.....\n");
//    printf("Thread %d open file %s successfully!!!!!!!!\n",block->index,block->filename);
    while( (block->start + byteswrite) < block->end)//receive messages from one client untill rec the sign of "exit"
    //while(data.start_pos != block->end)
    { 
//           pthread_mutex_lock(&client_mymutex_thread);
           //this is a different things that we should decalaration that makes things better
//           sem_wait(&sem);
           pthread_mutex_lock(&mymutex);
//               while(-1 == (pack_len = recv(sockfd,&data,sizeof(data),0)))
//               {    pack_len = recv(sockfd,&data,sizeof(data),0);
//                   printf("pack head size is %d\n",pack_len);
//               }
//               count++;
//               printf("thread %d :pack head size is %d\n",block->index,pack_len);

           printf("recv data by the %dth thread in the client!!\n",block->index);
           if(block->end - byteswrite < BLOCK_SIZE)
           {
               
               while(-1 == (len = recv(sockfd,char_recv,block->end - byteswrite,0)))
                   len = recv(sockfd,char_recv,block->end - byteswrite,0);
               printf("recv length is %d\n",strlen(char_recv));
           }
           else
           {
               while((len = recv(sockfd,char_recv,BLOCK_SIZE,0)) < BLOCK_SIZE)
               {
                   printf("the pre data size is %d\n",len);
                   len += (tmp = recv(sockfd,char_recv+len,BLOCK_SIZE - len,0));
                   printf("the expected data size is %d\n",BLOCK_SIZE - len);
                   if(len == BLOCK_SIZE || tmp == 0)
                     break;
                   printf("the last data size is %d\n",tmp);
               }
           }           
           pthread_mutex_unlock(&mymutex);

           printf("start to put data into the buf_1...\n");
           while(1)
           {
               if((tmp = myrecv(sockfd,buf_1,char_recv,len)) != 0)
               {    
                   break;
               }
           }
           memset(char_recv,0,BLOCK_SIZE);

//           pthread_mutex_unlock(&client_mymutex_thread);
/////////////////////////////////////////////////////////////////////////////

           byteswrite += len;
           tmp = 0;
           //printf("receive bytes : %d\n",strlen(char_recv));
//           printf("write data into the file from %ld\n totally %d bytes",data.start_pos,tmp);
           printf("Expected data size is %d,real size is %d\n",BLOCK_SIZE,len);
           printf("total size is %ld\n",byteswrite);
           printf("-------------------------------------------------------\n");
  //         sem_post(&sem);
    } 
    if(bufCnt != 0)
    {
        FILEWRITE = 1;
        pthread_cond_signal(&cond_1);
    }
    close(sockfd);
    //exit this thread of a client
    pthread_exit(NULL); 
 } 
/////////
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
//        printf("write thread open file %s successfully!!!!!!!!\n",block->filename);
//        pthread_mutex_lock(&buf_read);        
        pthread_mutex_lock(&client_mymutex_write);
        while(1)
        {
            while(FILEWRITE == 0)
            {    
                pthread_cond_wait(&cond_1,&client_mymutex_write);            
                printf("LOCK。。。...\n");
                break;
            }

//            printf("write thread circlying successfully!!!!!!!!\n");
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
	}
	printf("文件长度已找到(%d)，即将进入传送阶段，请稍后。。。\n",filesize);
        sleep(3);
        strcat(tem,filename);
        file->filename = tem;
        file->filesize = filesize > 0?filesize:0;
	return file;
}
