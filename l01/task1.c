#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WRITER_TURNS 10
#define READER_TURNS 10
#define READER_COUNT 5

pthread_mutex_t mutex;

pthread_mutex_t reader_count_mx;
volatile int reader_count = 0;

int getRandomTime(int limit) { return rand() % limit; }

// Writer thread function
int writer(void *data) {
  int i;

  for (i = 0; i < WRITER_TURNS; i++) {
    int result = pthread_mutex_lock(&mutex);
    if (result != 0) {
      fprintf(stderr, "Error occured during locking the mutex.\n");
      exit(-1);
    } else {
      // Write
      printf("(W) Writer started writing...\n");
      fflush(stdout);
      usleep(getRandomTime(800));
      printf("(W) Writer done writing\n");

      // Release ownership of the mutex object.
      result = pthread_mutex_unlock(&mutex);
      if (result != 0) {
        fprintf(stderr, "Error occured during unlocking the mutex.\n");
        exit(-1);
      }
      // Think, think, think, think
      usleep(getRandomTime(1000));
    }
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
      if (pthread_mutex_lock(&mutex) != 0) {
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
      if (pthread_mutex_unlock(&mutex) != 0) {
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

int main(int argc, char *argv[]) {
  srand(100005);

  pthread_t writerThread;
  pthread_t readerThreads[READER_COUNT];

  int i, rc;

  // Create the Writer thread
  rc = pthread_create(&writerThread,  // thread identifier
                      NULL,           // thread attributes
                      (void *)writer, // thread function
                      (void *)NULL);  // thread function argument

  if (rc != 0) {
    fprintf(stderr, "Couldn't create the writer thread");
    exit(-1);
  }

  // Create the Reader threads
  for (i = 0; i < READER_COUNT; i++) {
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

  // At this point, the readers and writers should perform their operations

  // Wait for the Readers
  for (i = 0; i < READER_COUNT; i++)
    pthread_join(readerThreads[i], NULL);

  // Wait for the Writer
  pthread_join(writerThread, NULL);

  return (0);
}
