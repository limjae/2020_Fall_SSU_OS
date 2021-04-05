#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>

#define SHARED 1

int data; // data variable

int item_to_produce, item_to_consume;
int p_loc=0, c_loc=0;
int total_consume=0;
int total_items, max_buf_size, num_workers, num_masters;
pthread_mutex_t mutx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t p_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_cond = PTHREAD_COND_INITIALIZER;

int *buffer;

void print_produced(int num, int master)
{
  printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker)
{
  printf("Consumed %d by worker %d\n", num, worker);
}

int is_full(int num)
{
  while ((p_loc + 1) % max_buf_size == c_loc % max_buf_size) // check if buffer is full
  {
    pthread_cond_wait(&c_cond, &mutx); // wait
  }
  if (item_to_produce >= total_items) // case for buffer is available but produce amout finished
  {
    pthread_cond_signal(&c_cond); //produce finished
    return 1;
  }
  return 0;
}

int is_empty(int num)
{
  while (c_loc % max_buf_size == p_loc % max_buf_size) // check if buffer is empty
  {
    if (total_consume >= total_items) // case for buffer is empty and consume amout finished
    {
      pthread_cond_signal(&p_cond); //consume finished
      return 1;
    }
    pthread_cond_wait(&p_cond, &mutx); // wait
  }
  return 0;
}

//produce items and place in buffer
//modify code below to synchronize correctly
void *generate_requests_loop(void *data)
{
  int thread_id = *((int *)data);
  while (1)
  {
    pthread_mutex_lock(&mutx);
    if(is_full(thread_id)==1)
    {
        pthread_mutex_unlock(&mutx);
        break;
    }
    buffer[p_loc % max_buf_size] = item_to_produce; //write
    print_produced(item_to_produce, thread_id);
    p_loc++;
    item_to_produce++; // actually same mean as p_loc
    pthread_cond_signal(&p_cond);
    pthread_mutex_unlock(&mutx);
  }
  return 0;
}

//consume items and place in buffer
//modify code below to synchronize correctly
void *generate_accept_loop(void *data)
{
  int thread_id = *((int *)data);
  while (1)
  {
    pthread_mutex_lock(&mutx);
    if (is_empty(thread_id))
    {
        
      pthread_mutex_unlock(&mutx);
      break;
    }
    item_to_consume = buffer[c_loc % max_buf_size]; //read
    c_loc++;
    print_consumed(item_to_consume, thread_id);
    total_consume++; //actually same mean as c_loc
    pthread_cond_signal(&c_cond);
    pthread_mutex_unlock(&mutx);
  }
  return 0;
}

//write function to be run by worker threads
//ensure that the workers call the function print_consumed when they consume an item

int main(int argc, char *argv[])
{
  pthread_mutex_init(&mutx, NULL);
  item_to_produce = 0;
  item_to_consume = 0;
  p_loc = 0;
  c_loc = 0;

  int *master_thread_id;
  pthread_t *master_thread;
  int *worker_thread_id;
  pthread_t *worker_thread;

  int i;
  if (argc < 5)
  {
    printf("./master-worker #total_items #max_buf_size #num_workers #masters e.g. ./exe 10000 1000 4 3\n");
    /*
    num_masters = 3;
    num_workers = 4;
    total_items = 100;
    max_buf_size = 10;
    */
    exit(1);
  }
  else
  {
    num_masters = atoi(argv[4]);
    num_workers = atoi(argv[3]);
    total_items = atoi(argv[1]);
    max_buf_size = atoi(argv[2]);
  }

  buffer = (int *)malloc(sizeof(int) * max_buf_size);

  //create master producer threads
  master_thread_id = (int *)malloc(sizeof(int) * num_masters);
  master_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);

  for (i = 0; i < num_masters; i++)
    master_thread_id[i] = i;
  for (i = 0; i < num_masters; i++)
    pthread_create(&master_thread[i], NULL, generate_requests_loop, (void *)&master_thread_id[i]);

  //create worker consumer threads
  worker_thread_id = (int *)malloc(sizeof(int) * num_workers);
  worker_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);

  for (i = 0; i < num_workers; i++)
    worker_thread_id[i] = i;
  for (i = 0; i < num_workers; i++)
    pthread_create(&worker_thread[i], NULL, generate_accept_loop, (void *)&worker_thread_id[i]); // worker_thread[i]

  //wait for all threads to complete
  for (i = 0; i < num_masters; i++)
  {
    pthread_join(master_thread[i], NULL);
    printf("master %d joined\n", i);
  }

  for (i = 0; i < num_workers; i++)
  {
    pthread_join(worker_thread[i], NULL);
    printf("worker %d joined\n", i);
  }
  pthread_mutex_destroy(&mutx);

  /*----Deallocating Buffers---------------------*/
  free(buffer);
  free(master_thread_id);
  free(master_thread);

  return 0;
}
