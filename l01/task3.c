#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WRITER_TURNS 10
#define READER_TURNS 10
#define WRITER_COUNT 5
#define READER_COUNT 5
#define OBJ_COUNT 3

typedef struct {
  pthread_mutex_t mutex;
  pthread_mutex_t reader_count_mx;
  int reader_count;
} shared_obj;

shared_obj objects[OBJ_COUNT];

int getRandomTime(int limit) { return rand() % limit; }

// Writer thread function
int writer(void *data) {
  int id = *((int*) data);
  for (int i = 0; i < WRITER_TURNS; i++) {
    int obj_index = rand() % OBJ_COUNT;
    shared_obj* obj = &objects[obj_index];
    printf("(W) Writer %d trying to write to %d\n", id, obj_index);
    int result = pthread_mutex_lock(&obj->mutex);
    if (result != 0) {
      fprintf(stderr, "Error occured during locking the mutex.\n");
      exit(-1);
    } else {
      // Write
      printf("(W) Writer %d started writing to %d...\n", id, obj_index);
      fflush(stdout);
      usleep(getRandomTime(800));
      printf("(W) Writer %d done writing to %d\n", id, obj_index);

      // Release ownership of the mutex object.
      result = pthread_mutex_unlock(&obj->mutex);
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
    int obj_index = rand() % OBJ_COUNT;
    shared_obj* obj = &objects[obj_index];
    printf("(R) Reader %d trying to read from %d\n", threadId, obj_index);
    if (pthread_mutex_lock(&obj->reader_count_mx) != 0) {
      fprintf(stderr, "Couldn't lock the reader count mutex.\n");
      exit(-1);
    }
    obj->reader_count++;
    if (obj->reader_count == 1) {
      if (pthread_mutex_lock(&obj->mutex) != 0) {
        fprintf(stderr, "Couldn't lock the mutex.\n");
        exit(-1);
      }
    }
    if (pthread_mutex_unlock(&obj->reader_count_mx) != 0) {
      fprintf(stderr, "Couldn't unlock the reader count mutex.\n");
      exit(-1);
    }
    // Read
    printf("(R) Reader %d started reading from %d...\n", threadId, obj_index);
    // Read, read, read
    usleep(getRandomTime(200));
    printf("(R) Reader %d finished reading from %d\n", threadId, obj_index);

    if (pthread_mutex_lock(&obj->reader_count_mx) != 0) {
      fprintf(stderr, "Couldn't lock the reader count mutex.\n");
      exit(-1);
    }
    obj->reader_count--;
    if (obj->reader_count == 0) {
      if (pthread_mutex_unlock(&obj->mutex) != 0) {
        fprintf(stderr, "Couldn't unlock the mutex.\n");
        exit(-1);
      }
    }
    if (pthread_mutex_unlock(&obj->reader_count_mx) != 0) {
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
