// copyright...

#ifndef SANE_NACL_UTIL_H__
#define SANE_NACL_UTIL_H__

#include <stdio.h>

#include <pthread.h>

namespace scanley {

class ScopedPthreadLock {
 public:
  explicit ScopedPthreadLock(pthread_mutex_t* mutex)
      : mutex_(mutex) {
    int rc = pthread_mutex_lock(mutex);
    if (rc)
      fprintf(stderr, "pthread_mutex_lock: %d (ERR)\n", rc);
  }
  ~ScopedPthreadLock() {
    int rc = pthread_mutex_unlock(mutex_);
    if (rc)
      fprintf(stderr, "pthread_mutex_unlock: %d (ERR)\n", rc);
  }
 private:
  pthread_mutex_t* mutex_;
};

class ScopedPthreadUnlock {
 public:
  explicit ScopedPthreadUnlock(pthread_mutex_t* mutex)
      : mutex_(mutex) {
    int rc = pthread_mutex_unlock(mutex);
    if (rc)
      fprintf(stderr, "pthread_mutex_unlock: %d (ERR)\n", rc);
  }
  ~ScopedPthreadUnlock() {
    int rc = pthread_mutex_lock(mutex_);
    if (rc)
      fprintf(stderr, "pthread_mutex_lock: %d (ERR)\n", rc);
  }
 private:
  pthread_mutex_t* mutex_;
};

template<typename Map, typename Key>
bool MapContainsKey(const Map& the_map, const Key& the_key) {
  return the_map.find(the_key) != the_map.end();
}

}  // namespace scanley

#endif
