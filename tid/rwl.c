#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "rwl.h"
#include "options.h"
#include "err.h"

typedef struct {
	pthread_mutex_t mutex;
	int putters;
	int getters;
	int putwaiters;
	int getwaiters;
	pthread_cond_t putable;
	pthread_cond_t getable;
} Lock;

static Lock lock;

extern void rwl_report() {
  if (lock.putters+lock.getters+lock.putwaiters>1)
    LOG("report(): putters=%d getters=%d putwaiters=%d",
	lock.putters,lock.getters,lock.putwaiters);
}

/***************************************************************************
 * This function initializes the lock.                                     *
 ***************************************************************************/
extern void rwl_init() {
  pthread_mutex_init(&lock.mutex,0);
  pthread_cond_init(&lock.putable,0);
  pthread_cond_init(&lock.getable,0);
  lock.getters=0;
  lock.putters=0;
  lock.putwaiters=0;
  lock.getwaiters=0;
}

/***************************************************************************
 * This function performs the locking for a get operation.                 *
 ***************************************************************************/
extern void rwl_get_lock() {
  pthread_mutex_lock(&lock.mutex);
  lock.getwaiters++;
  if (lock.putters || lock.putwaiters)
    do
      pthread_cond_wait(&lock.getable,&lock.mutex);
    while (lock.putters);
  lock.getwaiters--;
  lock.getters++;
  pthread_mutex_unlock(&lock.mutex);
}

/***************************************************************************
 * This function performs the locking for a put operation.                 *
 ***************************************************************************/
extern void rwl_put_lock() {
  pthread_mutex_lock(&lock.mutex);
  lock.putwaiters++;
  while (lock.getters || lock.putters)
    pthread_cond_wait(&lock.putable,&lock.mutex);
  lock.putwaiters--;
  lock.putters++;
  pthread_mutex_unlock(&lock.mutex);
}

/***************************************************************************
 * This function performs the unlocking for a get operation.               *
 ***************************************************************************/
extern void rwl_get_unlock() {
  pthread_mutex_lock(&lock.mutex);
  lock.getters--;
  pthread_cond_signal(&lock.putable);
  pthread_mutex_unlock(&lock.mutex);
}

/***************************************************************************
 * This function performs the unlocking for a put operation.               *
 ***************************************************************************/
extern void rwl_put_unlock() {
  pthread_mutex_lock(&lock.mutex);
  lock.putters--;
  if (lock.putwaiters && lock.getwaiters<=opt_queue())
    pthread_cond_signal(&lock.putable);
  else
    pthread_cond_broadcast(&lock.getable);
  pthread_mutex_unlock(&lock.mutex);
}
