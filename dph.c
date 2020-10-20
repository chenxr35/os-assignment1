#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#define NUM_THREADS 5
#define TRUE 1

void *philo(void *param);
void pickup_forks(int i);
void return_forks(int i);
void test(int i);
void func(int signum);

enum {THINKING,HUNGRY,EATING}state[5];

pthread_cond_t self[NUM_THREADS];// Behalf on each philosophers
pthread_mutex_t mutex[NUM_THREADS]; // For conditional variables
pthread_t tid[NUM_THREADS]; /*the thread identifier*/
int id[NUM_THREADS]={0,1,2,3,4};// For philo function


int main(int argc,char*argv[]){
  int i;
  int *ptr=id;

  pthread_attr_t attr; /*set of thread attributes*/
  /*get the default attributes*/
  pthread_attr_init(&attr);

  /*create the thread*/
  for(i=0;i<NUM_THREADS;i++)
    pthread_create(&tid[i],&attr,philo,ptr+i);
 
 /*wait for the thread to exit*/
  for(i=0;i<NUM_THREADS;i++)
    pthread_join(tid[i],NULL);
  signal(SIGINT,func); 
}

/*The thread will begin control in this function*/
void *philo(void *param){
    do{
    int id = *( (int *)param);
    /* Try to pickup a chopstick*/
    pickup_forks(id);
    printf("The philosopher %d is eating...\n",id);
    /* Eat a while*/
    srand((unsigned)time(NULL));
    int sec = (rand()%((3-1)+1)) +1;// make sec in [1,3]
    sleep(sec);
    /* Return a chopstick */
    return_forks(id);
    printf("The philosopher %d is thinking...\n",id);
    /* Think a while */
    srand((unsigned)time(NULL));
    sec = (rand()%((3-1)+1)) +1;// make sec in [1,3]
    sleep(sec);
    }while(TRUE);
    pthread_exit(NULL);
}

void pickup_forks(int i){
    state[i] = HUNGRY;// Wants to eat
    test(i);// Check can eat or not
    pthread_mutex_lock(&mutex[i]);
    while (state[i] != EATING){
        pthread_cond_wait(&self[i],&mutex[i]);//Wait his neighbors ate
    }
    pthread_mutex_unlock(&mutex[i]);
}

void return_forks(int i){
    state[i] = THINKING;
    //Notify his neighbor that I was ate.
    test((i+4)%5);
    test((i+1)%5);
}

void test(int i){
    // A philosopher can eat when he wants to eat and his neighbors aren't eating.
    if ( (state[(i+4)%5] != EATING)&&
    (state[i] == HUNGRY) &&
    (state[(i+1)%5] != EATING)){
        pthread_mutex_lock(&mutex[i]);
        state[i] = EATING;
        pthread_cond_signal(&self[i]);
        pthread_mutex_unlock(&mutex[i]);
    }
}

void func(int signum){
    printf("\nKilling the philosophers...\n");
    for (int i=0;i<5;i++){
        pthread_cancel(tid[i]);
    }
    for (int i=0;i<5;i++){
        pthread_cond_destroy(&self[i]);
        pthread_mutex_destroy(&mutex[i]);
    }
    printf("Killed philosophers.\n");
}
