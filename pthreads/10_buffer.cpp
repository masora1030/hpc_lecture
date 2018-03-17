#include <stdio.h>
#include <pthread.h>

#define EMPTY -2           // buffer slot has nothing in it
#define END -1             // consumer who grabs this should exit
const int loops=2;         // number of producer loops
const int producers=2;     // number of producers
const int consumers=2;     // number of consumers
const int num_threads=producers+consumers;
const int max_buffer=3;    // maximum capacity of buffer
int begin_p=0;             // tracks where next consume should come from
int end_p=0;               // tracks where next produce should go to
int count=0;               // counts how many entries are full
int *buffer;               // the buffer itself: malloc in main()
pthread_cond_t empty=PTHREAD_COND_INITIALIZER;
pthread_cond_t fill=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void print_headers(int producers, int consumers) {
  printf("N   ");
  for (int i=0; i<max_buffer; i++) {
    printf("   ");
  }
  for (int i=0; i<producers; i++)
    printf(" P%d  ", i);
  for (int i=0; i<consumers; i++)
    printf(" C%d  ", i);
  printf("\n");
}

void print(int thread_id, const char *str) {
  printf("%d [", count);
  for (int i=0; i<max_buffer; i++) {
    if (begin_p == i && end_p == i) {
      printf("*");
    } else if (begin_p == i) {
      printf("b");
    } else if (end_p == i) {
      printf("e");
    } else {
      printf(" ");
    }
    if (buffer[i] == EMPTY) {
      printf("%s ", "-");
    } else if (buffer[i] == END) {
      printf("%s ", "E");
    } else {
      printf("%d ", buffer[i]);
    }
  }
  printf("] ");
  for (int i=0; i<thread_id; i++) {
    printf("     ");
  }
  printf("%4s\n", str);
}

void put(int value) {
  buffer[end_p]=value;
  end_p=(end_p + 1) % max_buffer;
  count++;
}

int get() {
  int tmp=buffer[begin_p];
  buffer[begin_p]=EMPTY;
  begin_p=(begin_p + 1) % max_buffer;
  count--;
  return tmp;
}

void *producer(void *arg) {
  int id=(size_t) arg;
  int base=id * loops;
  for (int i=0; i<loops; i++) {
    pthread_mutex_lock(&mutex); print(id, "lock");
    while (count == max_buffer) { print(id, "full"); print(id, "uloc");
      pthread_cond_wait(&empty, &mutex); print(id, "resu"); print(id, "lock");
    }
    put(base + i); print(id, "put ");
    pthread_cond_signal(&fill); print(id, "uloc");
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

void *consumer(void *arg) {
  int id=(size_t) arg;
  int tmp=0;
  while (tmp != END) {
    pthread_mutex_lock(&mutex); print(id, "lock");
    while (count == 0) { print(id, "none"); print(id, "uloc");
      pthread_cond_wait(&fill, &mutex); print(id, "resu"); print(id, "lock");
    }
    tmp=get(); print(id, "get ");
    pthread_cond_signal(&empty); print(id, "uloc");
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  buffer=new int [max_buffer];
  for (int i=0; i<max_buffer; i++) {
    buffer[i]=EMPTY;
  }
  print_headers(producers, consumers);
  pthread_t pid[num_threads], cid[num_threads];
  size_t thread_id=0;
  for (int i=0; i<producers; i++) {
    pthread_create(&pid[i], NULL, producer, (void *) thread_id);
    thread_id++;
  }
  for (int i=0; i<consumers; i++) {
    pthread_create(&cid[i], NULL, consumer, (void *) thread_id);
    thread_id++;
  }
  for (int i=0; i<producers; i++) {
    pthread_join(pid[i], NULL);
  }
  for (int i=0; i<consumers; i++) {
    pthread_mutex_lock(&mutex);
    while (count == max_buffer)
      pthread_cond_wait(&empty, &mutex);
    put(END);
    pthread_cond_signal(&fill);
    pthread_mutex_unlock(&mutex);
  }
  for (int i=0; i<consumers; i++) {
    pthread_join(cid[i], NULL);
  }
  delete[] buffer;
  return 0;
}
