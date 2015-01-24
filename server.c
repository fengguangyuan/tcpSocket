    /***************************************************
    * FILENAME：pthread_server.c
    * DESCRIPTION：创建子线程来接收客户端的数据
    * COPYRIGHT:SA14226417 Mr.feng
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
    #include <time.h>

    #define MAX_SIZE 256
    
     /*******************************************************
     * PROCESS: control the main thread and the sub threads
     * 1: main thread
     * 0: sub threads    
     **/
    int PROCESS = 1;

    /********************************************************
     * build the communication info between
     * the server and the client
     */
    struct sockaddr_in server_address; 
    struct sockaddr_in client_address; 
    unsigned int PORT = 8888;
    int server_sockfd;
    int *client_sockfd; 
    int server_len,client_len; 

    /********************************************************
     * the globle vars used in this file
     */
    int u_count = 0;                   //the number of users
    int count = 0;                     //sending counts
    int bytesread = 0;                 //reading counts

    /*declaration for the callee func*/
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
           {  printf("bind failed\n");           //绑定套接字 
              exit(0);
           }

           int nNoDelay=1;                       //设置为no delay
           setsockopt(server_sockfd,SOL_SOCKET,SO_SNDBUF,(const char*)&nNoDelay,sizeof(int));

           if(-1 == listen(server_sockfd,20))
           {  printf("listen failed\n");exit(0);
           }
           templen = sizeof(struct sockaddr); 
           printf("server waiting for connect\n");  
           while(1){ 
               pthread_t thread[THREAD_NUM];     //创建不同的子线程以区别不同的客户端 
               client_sockfd = (int *)malloc(sizeof(int)); 
               client_len = sizeof(client_address); 
               *client_sockfd = accept(server_sockfd,(struct sockaddr *)&client_address, (socklen_t *)&client_len); 
               if(-1 == *client_sockfd){ 
                      perror("accept"); 
                      shutdown(*client_sockfd,2);//结束本机连接
                      continue; 
               }
               //用户数量加 1
               u_count++;
               
               printf("the %d th user has been connected！！！！\n",u_count);
               //发送成功连接信息给客户端
               send(*client_sockfd,"connect successfully!!!!\n",25,0);
               //获取要传输的文件
               if(PROCESS == 1)                   //main thread
               {
                  while(-1==recv(*client_sockfd,filename,256,0))
                  printf("file name is : %s\n",filename);
                  printf("file name is : %s\n",filename);                  
                  //获取文件大小,传送给客记端
                  if( stat(filename,&buf))        //没有找到文件就关闭此次链接
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
               else                                       //sub thread
               {
                    //线程属性设置为NULL
                    block[i].filename = filename;         //file name
                    block[i].start = i * block_size;      //start address in the file
                    block[i].end = filesize;              //file size
                    block[i].index = i;                   //thread no
                    block[i].sockfd = client_sockfd;      //thread socket
                    //创建子线程 
                    if(pthread_create(&thread[i], NULL, send_data, &block[i])!=0)
                    { 
                           perror("pthread_create\n"); 
                           break; 
                    } 
                    //thread number
                    i+=1;
               }//end if
           } //end while
           shutdown(*client_sockfd,2); 
           shutdown(server_sockfd,2); 
    }  

    /*****************************************
    * FUNCTION：rec_data_one
    * DESCRIPTION：接受客户端的数据
    * ARGUMENTS:
    *	arg: accept a pointer of thread_block
    *****************************************/ 
    void *send_data(void *arg) 
    { 
           struct thread_block *block = (struct thread_block*)arg;
           struct Data_Block *data;
           long current_pos;
           int len,pack_len = 0;
           char char_send[BLOCK_SIZE]={0};//存放数据
           time_t t_start,t_end;
           memset(char_send,0,BLOCK_SIZE);
           //test time start
           t_start = time(NULL);  

           printf("the %d th thread has been created, then exacute this thread\n",u_count);
           printf("Thread %d start!!!!!!!!!!!!!!!!\n",block->index);
           printf("Open file ==> %s\n",block->filename);
           printf("this thread send data frome %ld\n",block->start);
           printf("this thread send data end withe %ld\n",block->end);
           FILE *fp;
           if((fp = fopen(block->filename,"rb")) == NULL)
               perror("file open failed!!!!!\n");
           while(current_pos != block->end)
           { 
           //this is a different things that we should decalaration that makes things better
               pthread_mutex_lock(&mymutex_read);
               if(BLOCK_SIZE > block->end - current_pos)
               {
                   if(bytesread == block->end)
                       break;
                   len = fread(char_send,1,block->end-current_pos,fp);
                   current_pos = ftell(fp);
               }else
               {
                   len =fread(char_send,1,BLOCK_SIZE,fp);
                   current_pos = ftell(fp);
               }
	             bytesread += len;
               pthread_mutex_unlock(&mymutex_read);
               //给客户端相应的线程传数据
               pthread_mutex_lock(&mymutex);
                  while(-1 == (pack_len = send(*(block->sockfd),&data,sizeof(data),0)))
                   {    pack_len = send(*(block->sockfd),&data,sizeof(data),0);
                   }

               //two thread with sleep(0.02)
               if(len != 0)
                   while(-1 == (len = (send(*(block->sockfd),char_send,len,0))))
                   {
                       len = send(*(block->sockfd),char_send,len,0);
                   }
               memset(char_send,0,BLOCK_SIZE);
               pthread_mutex_unlock(&mymutex);
           } 
           t_end = time(NULL);
           printf("time cost is %f\n",difftime(t_start,t_end));

           fclose(fp);
           close(*(block->sockfd));
           free(block->sockfd); 
           //exit this thread of a client
           pthread_exit(NULL); 
} 
