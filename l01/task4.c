#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define WRITER_TURNS 10
#define READER_TURNS 10
#define WRITER_COUNT 5
#define READER_COUNT 5
#define OBJ_COUNT 3
#define CRITIC_TURNS (WRITER_TURNS * WRITER_COUNT)

pthread_mutex_t mutex;
pthread_mutex_t reader_count_mx;
pthread_mutex_t dummy_mutex;
int reader_count;
bool awaiting_critique = false;
pthread_cond_t critic_cond;

int getRandomTime(int limit) { return rand() % limit; }

// Writer thread function
int writer(void *data) {
  int id = *((int*) data);
  for (int i = 0; i < WRITER_TURNS; i++) {
    printf("(W) Writer %d trying to write\n", id);
    int result = pthread_mutex_lock(&mutex);
    if (result != 0) {
      fprintf(stderr, "Error occured during locking the mutex.\n");
      exit(-1);
    } else {
      // Write
      printf("(W) Writer %d started writing...\n", id);
      fflush(stdout);
      usleep(getRandomTime(800));
      printf("(W) Writer %d done writing\n", id);

      awaiting_critique = true;
      pthread_cond_signal(&critic_cond);
      // Think, think, think, think
      usleep(getRandomTime(1000));
    }
  }

  return 0;
}

void* critic(void* data) {
  for(size_t i = 0; i < CRITIC_TURNS; i++) {
    if (pthread_mutex_lock(&dummy_mutex) != 0) {
      fprintf(stderr, "Critic couldn't lock the dummy mutex\n");
      exit(EXIT_FAILURE);
    }

    while(!awaiting_critique) {
      pthread_cond_wait(&critic_cond, &dummy_mutex);
    }

    if (pthread_mutex_unlock(&dummy_mutex) != 0) {
      fprintf(stderr, "Critic couldn't unlock the dummy mutex\n");
      exit(EXIT_FAILURE);
    }

    printf("(C) Criticizing...\n");
    usleep(getRandomTime(1000));
    awaiting_critique = false;
    printf("(C) Done\n");

    if (pthread_mutex_unlock(&mutex)) {
      fprintf(stderr, "Critic couldn't unlock the mutex\n");
    }
  }

  return NULL;
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
    printf("(R) Reader %d finished reading\n", threadId);

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

  pthread_t writerThreads[WRITER_COUNT];
  pthread_t readerThreads[READER_COUNT];
  pthread_t criticThread;

  int i, rc;

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

  pthread_create(&criticThread, NULL, critic, NULL);

  // At this point, the readers and writers should perform their operations

  // Wait for the Readers
  for (i = 0; i < READER_COUNT; i++){
    pthread_join(readerThreads[i], NULL);
  }

  // Wait for the Writer
  for(size_t i = 0; i < WRITER_COUNT; i++) {
    pthread_join(writerThreads[i], NULL);
  }

  return (0);
}
