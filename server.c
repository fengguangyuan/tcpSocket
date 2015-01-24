    /***************************************************
    * 文件名：pthread_server.c
    * 文件描述：创建子线程来接收客户端的数据
    ***************************************************/ 
    #include "file_ops.h"    
    #include <sys/types.h> 
    #include <sys/stat.h>
    #include <sys/socket.h>
    #include <netinet/tcp.h> 
    #include <stdio.h> 
    #include <netinet/in.h> 
    #include <arpa/inet.h> 
    #include <unistd.h> 
    #include <stdlib.h> 
    #include <pthread.h> 
    #include <string.h>

    #define MAX_SIZE 256
    #define BUFF_LEN 1024
    int PROCESS = 1;
    struct sockaddr_in server_address; 
    struct sockaddr_in client_address; 
    struct sockaddr_in tempaddr; 
    int server_sockfd;
    int *client_sockfd; 
    int server_len,client_len; 
    int u_count = 0;//the number of users
    unsigned int PORT = 8888;
    void *send_data(void *block);
    int main(int argc,char *argv[]) 
    { 
           int i = 0,byte; 
           char char_recv,char_send; 
           socklen_t templen;
           char filename[256] = {0};
           struct stat buf;
           size_t filesize;
           struct thread_block block[THREAD_NUM];
           long block_size;
           server_sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建套接字 
           if(!server_sockfd)
             printf("server_socket is failed......");
           bzero(&server_address,sizeof(struct sockaddr_in));
           server_address.sin_family = AF_INET; 
           server_address.sin_addr.s_addr =  htonl(INADDR_ANY); 
           server_address.sin_port = htons(PORT); 
           server_len = sizeof(struct sockaddr); 
           
           if(-1 == bind(server_sockfd, (struct sockaddr *)(&server_address), sizeof(struct sockaddr)))
           {  printf("bind failed\n");//绑定套接字 
              exit(0);
           }

           int nNoDelay=1;
           setsockopt(server_sockfd,SOL_SOCKET,SO_SNDBUF,(const char*)&nNoDelay,sizeof(int));

           if(-1 == listen(server_sockfd,20))
           {  printf("listen failed\n");exit(0);
           }
           templen = sizeof(struct sockaddr); 
           printf("server waiting for connect\n");  
           while(1){ 
                  pthread_t thread[THREAD_NUM];//创建不同的子线程以区别不同的客户端 
                  client_sockfd = (int *)malloc(sizeof(int)); 
                  client_len = sizeof(client_address); 
                  *client_sockfd = accept(server_sockfd,(struct sockaddr *)&client_address, (socklen_t *)&client_len); 
                  if(-1 == *client_sockfd){ 
                         //perror("accept"); 
                         shutdown(*client_sockfd,2);//结束本机连接
                         continue; 
                  }
                  u_count++;//用户数量加 1
                  printf("the %d th user has been connected！！！！\n",u_count);
		              //发送成功连接信息给客户端
                  send(*client_sockfd,"connect successfully!!!!\n",25,0);
            		  //获取要传输的文件
                  if(PROCESS == 1)
                  {
		                while(-1==recv(*client_sockfd,filename,256,0))
                    printf("file name is : %s\n",filename);
                    printf("file name is : %s\n",filename);                  
                    //获取文件大小,传送给客记端
                    if( stat(filename,&buf))  //没有找到文件就关闭此次链接
                    {
                      perror("file searching failed!!!!\n");
                      close(*client_sockfd);
                      continue;
                    }
                    printf("file size is : %ld\n",buf.st_size);
                    block_size = buf.st_size / THREAD_NUM;
                    filesize = buf.st_size;
		                send(*client_sockfd,&filesize,sizeof(size_t),0);
                    PROCESS = 0;
                  }
                  else
                  {                    
	     	          //线程属性设置为NULL
	                  block[i].filename = filename;
	                  block[i].start = i * block_size;
		                if(block_size > buf.st_size - block[i].start)
		                  block[i].end = buf.st_size - block[i].start;
		                else 
	    	              block[i].end = block[i].start + block_size;
		                block[i].index = i;
                    block[i].sockfd = client_sockfd;
                    if(pthread_create(&thread[i], NULL, send_data, &block[i])!=0)//创建子线程 
                    { 
                           perror("pthread_create\n"); 
                           break; 
                    } 
                    //printf("the %d th thread has been created, then exacute this thread\n",u_count);
		                //printf("Currently, %d users are online\n\n",u_count);
                    i+=1;
          }
       } 
           shutdown(*client_sockfd,2); 
           shutdown(server_sockfd,2); 
    }  

    /*****************************************
    * 函数名称：rec_data_one
    * 功能描述：接受客户端的数据
    * 参数列表：fd——连接套接字
    * 返回结果：void *
    *****************************************/ 
    void *send_data(void *arg) 
    { 
           struct thread_block *block = (struct thread_block*)arg;
           long bytesread = 0,next_pos;
           int len,pack_len = 0;
	         char char_send[BUFF_LEN]={0};//存放数据
           struct Data_Block data;
           data.start_pos = 0;
  
           printf("the %d th thread has been created, then exacute this thread\n",u_count);
           printf("Thread %d start!!!!!!!!!!!!!!!!\n",block->index);
           printf("Open file ==> %s\n",block->filename);
           FILE *fp;
           if((fp = fopen(block->filename,"rt")) == NULL)
               perror("file open failed!!!!!\n");
           //frewind(fp,0);
           rewind(fp);
           fseek(fp,block->start,0);
           data.start_pos = block->start;	
           while( (block->start + bytesread) != block->end)//receive messages from one client untill rec the sign of "exit"
           { 
           //this is a different things that we should decalaration that makes things better
               fseek(fp,next_pos,0);
               if(BUFF_LEN > block->end - block->start)
               {
  //              pthread_mutex_lock(&mymutex_read);
                  len = fread(char_send,1,block->end-block->start,fp);
	                //pthread_mutex_unlock(&mymutex_read);
                  sleep(1);
                  printf("_---------------------------------%d\n",strlen(char_send));
               }else
               {
                  //pthread_mutex_lock(&mymutex_read); 
                  len =fread(char_send,1,BUFF_LEN,fp);
                  // sleep(1);
                  printf("----------------------------------%d\n",strlen(char_send));
                  //pthread_mutex_unlock(&mymutex_read);
               }
	             //给客户端相应的线程传数据
	             printf("send dat to the %dth thread in the client!!\n",block->index);
               next_pos = ftell(fp);

               pthread_mutex_lock(&mymutex_write);
               while(-1 == (pack_len = send(*(block->sockfd),&data,sizeof(data),0)))
               {    pack_len = send(*(block->sockfd),&data,sizeof(data),0);
                   printf("pack head size is %d\n",pack_len);
               }
               printf("pack head size is %d\n",pack_len);
               pack_len = 0;
               //sleep(0.1);
               while(-1 == (len = (send(*(block->sockfd),char_send,len,0))))
	             {
		              len = send(*(block->sockfd),char_send,len,0);
                  printf("pack head size is %d\n",pack_len);
	             }
               data.start_pos = next_pos;
               pthread_mutex_unlock(&mymutex_write);

	             bytesread += len;
               memset(char_send,0,BUFF_LEN);
               printf("Thread %d start sending data from \"%ld\"\n",block->index,block->start);
               printf("file pointer %ld\n",data.start_pos);
               printf("real sending data size is %d\n",len);
//             printf("Thread %d send data bytes ====> %ld\n",block->index,bytesread);
               printf("Expected size is : %ld\n",block->end - block->start);
//             sleep(1);
           } 
           fclose(fp);
           close(*(block->sockfd));
           free(block->sockfd); 
           //exit this thread of a client
           pthread_exit(NULL); 
} 
