#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WRITER_TURNS 5
#define READER_TURNS 5
#define WRITER_COUNT 3
#define READER_COUNT 3

#define BUFFER_TURNS (WRITER_TURNS * WRITER_COUNT)

#define BUFFER_SIZE 2

pthread_mutex_t buffer_mutex;
pthread_mutex_t write_mutex;

pthread_mutex_t reader_count_mx;
volatile int reader_count = 0;

volatile int buffer_available = 0;

pthread_cond_t buffer_write_cond;
pthread_cond_t buffer_read_cond;

int getRandomTime(int limit) { return rand() % limit; }

// Writer thread function
int writer(void *data) {
  int id = *((int*) data);
  for (size_t i = 0; i < WRITER_TURNS; i++) {
    if (pthread_mutex_lock(&buffer_mutex) != 0) {
      fprintf(stderr, "Error occured during locking the mutex.\n");
      exit(-1);
    }

    while(buffer_available >= BUFFER_SIZE) {
      pthread_cond_wait(&buffer_write_cond, &buffer_mutex);
    }

    // Write
    printf("(W) Writer %d started writing...\n", id);
    fflush(stdout);
    usleep(getRandomTime(800));
    printf("(W) Writer %d done writing\n", id);

    buffer_available++;
    pthread_cond_broadcast(&buffer_read_cond);

    printf("Items in buffer: %d\n", buffer_available);

    // Release ownership of the mutex object.
    if (pthread_mutex_unlock(&buffer_mutex) != 0) {
      fprintf(stderr, "Error occured during unlocking the mutex.\n");
      exit(-1);
    }
    // Think, think, think, think
    usleep(getRandomTime(1000));
  }

  return 0;
}

// Reader thread function
int reader(void *data) {
  int threadId = *(int *)data;

  for (int i = 0; i < READER_TURNS; i++) {
    printf("(R) Reader %d trying to read\n", threadId);
    if (pthread_mutex_lock(&reader_count_mx) != 0) {
      fprintf(stderr, "Couldn't lock the reader count mutex.\n");
      exit(-1);
    }
    reader_count++;
    if (reader_count == 1) {
      if (pthread_mutex_lock(&write_mutex) != 0) {
        fprintf(stderr, "Couldn't lock the mutex.\n");
        exit(-1);
      }
    }
    if (pthread_mutex_unlock(&reader_count_mx) != 0) {
      fprintf(stderr, "Couldn't unlock the reader count mutex.\n");
      exit(-1);
    }
    // Read
    printf("(R) Reader %d started reading...\n", threadId);
    // Read, read, read
    usleep(getRandomTime(200));
    printf("(R) Reader %d finished\n", threadId);

    if (pthread_mutex_lock(&reader_count_mx) != 0) {
      fprintf(stderr, "Couldn't lock the reader count mutex.\n");
      exit(-1);
    }
    reader_count--;
    if (reader_count == 0) {
      if (pthread_mutex_unlock(&write_mutex) != 0) {
        fprintf(stderr, "Couldn't unlock the mutex.\n");
        exit(-1);
      }
    }
    if (pthread_mutex_unlock(&reader_count_mx) != 0) {
      fprintf(stderr, "Couldn't unlock the reader count mutex.\n");
      exit(-1);
    }

    usleep(getRandomTime(800));
  }

  free(data);

  return 0;
}

int buffer(void *data) {
  int i;

  for (i = 0; i < BUFFER_TURNS; i++) {
    if (pthread_mutex_lock(&buffer_mutex) != 0) {
      fprintf(stderr, "couldn't lock the buffer.\n");
      exit(-1);
    }
    while (buffer_available <= 0) {
      pthread_cond_wait(&buffer_read_cond, &buffer_mutex);
    }
    buffer_available--;
    pthread_cond_broadcast(&buffer_write_cond);
    printf("Buffer popped, available: %d\n", buffer_available);

    if (pthread_mutex_unlock(&buffer_mutex) != 0) {
      fprintf(stderr, "couldn't unlock the buffer.\n");
      exit(-1);
    }

    if (pthread_mutex_lock(&write_mutex) != 0) {
      fprintf(stderr, "couldn't unlock for writer.\n");
      exit(-1);
    }
    // Write
    printf("(W) Buffer started writing...\n");
    fflush(stdout);
    usleep(getRandomTime(800));
    printf("(W) Buffer done writing\n");

    // Release ownership of the mutex object.
    if (pthread_mutex_unlock(&write_mutex) != 0) {
      fprintf(stderr, "Error occured during unlocking the mutex.\n");
      exit(-1);
    }
    // Think, think, think, think
    usleep(getRandomTime(1000));
  }

  return 0;
}


int main(int argc, char *argv[]) {
  srand(100005);

  pthread_t writerThreads[WRITER_COUNT];
  pthread_t readerThreads[READER_COUNT];
  pthread_t buffer_thread;

  int rc;

  // Create the Writer thread
  for(size_t i = 0; i < WRITER_COUNT; i++) {
    int *threadId = malloc(sizeof(int));
    *threadId = i;
    rc = pthread_create(&writerThreads[i],
                        NULL,
                        (void*) writer,
                        (void*) threadId);
    if (rc != 0) {
      fprintf(stderr, "Couldn't create the writer thread");
      exit(-1);
    }
  }

  // Create the Reader threads
  for (size_t i = 0; i < READER_COUNT; i++) {
    // Reader initialization - takes random amount of time
    usleep(getRandomTime(1000));
    int *threadId = malloc(sizeof(int));
    *threadId = i;
    rc = pthread_create(&readerThreads[i], // thread identifier
                        NULL,              // thread attributes
                        (void *)reader,    // thread function
                        (void *)threadId); // thread function argument

    if (rc != 0) {
      fprintf(stderr, "Couldn't create the reader threads");
      exit(-1);
    }
  }

  rc = pthread_create(&buffer_thread, NULL, (void*) buffer, NULL);

  // At this point, the readers and writers should perform their operations

  // Wait for the Readers
  for (size_t i = 0; i < READER_COUNT; i++) {
    pthread_join(readerThreads[i], NULL);
  }

  // Wait for the Writer
  for(size_t i = 0; i < WRITER_COUNT; i++) {
    pthread_join(writerThreads[i], NULL);
  }

  pthread_join(buffer_thread, NULL);

  return (0);
}
