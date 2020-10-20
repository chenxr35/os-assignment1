#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <math.h>
#include <signal.h>
#define NUM_THREADS 3
#define BUFFER_SIZE 20

typedef int buffer_item;
struct buf{
    int rear;
    int front;
    buffer_item buffer[BUFFER_SIZE];
};

void *ptr;
void *consumer(void *param);
double produce_time(double lambda_p);
void func(int signum);

sem_t *full;
sem_t *empty;
sem_t *s_mutex;
pthread_t tid[NUM_THREADS];

int main(int argc,char *argv[])
{ 
  struct buf sm;
  memset(&sm,0,sizeof(struct buf));
  int lambda_c=atof(argv[1]);
  int *import =&lambda_c;
  full = sem_open("full",O_CREAT);//link the semaphore from producer
  empty = sem_open("empty",O_CREAT);
  s_mutex = sem_open("mutex",O_CREAT);
  int shm_fd = shm_open("buffer",O_RDWR,0666);
  ptr = mmap(0,sizeof(struct buf),PROT_READ | PROT_WRITE,MAP_SHARED,shm_fd,0); // read the shared memory from producer

  int i,scope;
  pthread_attr_t attr;

  pthread_attr_init(&attr);

  if(pthread_attr_getscope(&attr,&scope)!=0)
    fprintf(stderr,"Unable to get scheduling scope\n");
  else{
    if(scope==PTHREAD_SCOPE_PROCESS)
      printf("PTHREAD_SCOPE_PROCESS\n");
    else if(scope==PTHREAD_SCOPE_SYSTEM)
      printf("PTHREAD_SCOPE_SYSTEM\n");
    else
      fprintf(stderr,"Illegal scope value.\n");
  }

  pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);

  for(i=0;i<NUM_THREADS;i++)
    pthread_create(&tid[i],&attr,consumer,import);

  for(i=0;i<NUM_THREADS;i++)
    pthread_join(tid[i],NULL);

}

void *consumer(void *param){
    int lambda_c = *(int *)param;
    do{
        double interval_time =(double)lambda_c;
        sleep((unsigned int)produce_time(interval_time));
        struct buf *shm_ptr = ((struct buf*)ptr);
        sem_wait(full);//Wait for a full buffer
        sem_wait(s_mutex);
        buffer_item item = shm_ptr->buffer[shm_ptr->front];
        shm_ptr->front = (shm_ptr->front+1) % BUFFER_SIZE;
        sem_post(s_mutex);
        sem_post(empty);//Add a empty buffer
        printf("Consuming the data %d by pid %ld,%ld\n",item,(long)getpid(),(long)pthread_self());
    }while (1);
    pthread_exit(0);
}

double produce_time(double lambda_p){
    double z;
    do
    {
        z = ((double)rand() / RAND_MAX);
    }
    while((z==0) || (z == 1));
    return (-1/lambda_p * log(z));
}

void func(int signum){
    printf("\nKilling the consumers...\n");
    for (int i=0;i<NUM_THREADS;i++){
        pthread_cancel(tid[i]);
    }
    sem_close(full);
    sem_close(empty);
    sem_close(s_mutex);
    sem_unlink("full");
    sem_unlink("empty");
    sem_unlink("mutex");
    shm_unlink("buffer");
    printf("Killed consumers.\n");
}
