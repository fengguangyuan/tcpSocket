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

#define THREAD_NUM 1
#define BLOCK_SIZE 1024
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mymutex_read = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mymutex_write = PTHREAD_MUTEX_INITIALIZER;
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


 
