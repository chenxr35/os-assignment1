#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <math.h>
#include <signal.h>
#define NUM_THREADS 3
#define BUFFER_SIZE 20

void *producer(void *param);
double produce_time(double lambda_p);
void func(int signum);

void *ptr;

sem_t *full;
sem_t *empty;
sem_t *s_mutex;//mutex for struct
pthread_t tid[NUM_THREADS];

typedef int buffer_item;
struct buf{
    int rear;
    int front;
    buffer_item buffer[BUFFER_SIZE];
};

int main(int argc,char *argv[])
{ 
  struct buf sm;
  memset(&sm,0,sizeof(struct buf));

  int lambda_p = atof(argv[1]);
  int *import =&lambda_p;

  full = sem_open("full",O_CREAT,0666,0);//sem_open returns a sem_t pointer
  empty = sem_open("empty",O_CREAT,0666,BUFFER_SIZE);
  s_mutex = sem_open("mutex",O_CREAT,0666,1);
  
  int shm_fd = shm_open("buffer",O_CREAT | O_RDWR,0666); //Create the shared memory
  ftruncate(shm_fd,sizeof(struct buf));
  ptr = mmap(0,sizeof(struct buf),PROT_WRITE,MAP_SHARED,shm_fd,0);//Mapped it into file, which can shared by different process

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
    pthread_create(&tid[i],&attr,producer,import);
  for(i=0;i<NUM_THREADS;i++)
    pthread_join(tid[i],NULL);
  signal(SIGINT,func);
}

void *producer(void *param){
    int lambda_p = *(int *)param;
    do{
        double interval_time =(double)lambda_p;
        usleep((unsigned int)(produce_time(interval_time)*1e6)); // sleep the time by the distribution of negative exponetial.
        buffer_item item = rand() % 255;
        struct buf *shm_ptr = ((struct buf *)ptr);// read the round queue's information from shared memory.
        sem_wait(empty); // If there is no empty buffer, block.
        sem_wait(s_mutex);//lock the binary-mutex and execute the code in critical area.
        printf("Producing the data %d to buffer[%d] by id %ld,%ld\n",item,shm_ptr->rear,(long)getpid(),(long)pthread_self());
        shm_ptr->buffer[shm_ptr->rear] = item; // Put the data to round queue.
        shm_ptr->rear = (shm_ptr->rear+1) % BUFFER_SIZE;
        sem_post(s_mutex);//Unlock the binary-mutex
        sem_post(full);// Add a full buffer
    }while(1);
    printf("end\n");
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
    printf("\nKilling the producers...\n");
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
    printf("Killed producers.\n");
}
