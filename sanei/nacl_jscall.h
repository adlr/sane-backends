// copyright

#include <pthread.h>

#include "ppapi/cpp/instance.h"

class SynchronousJavaScriptCaller {
 public:
  SynchronousJavaScriptCaller() : next_id_(0) {
    pthread_mutex_init(&mutex_, NULL);
  }

  // pseudocode
  post() {
    ScopedPthreadLock lock(&mutex_);
    while (!outbox_.empty()) {
      pair<size_t, string> msg = outbox_.begin();
      outbox_.erase(outbox_.begin());

      ScopedPthreadUnlock unlock(&mutex_);

      // send msg to javascript
    }
  }

  std::string Call(const std::string& request) {
    ScopedPthreadLock lock(&mutex_);
    size_t idx = next_id_++;
    outbox_[idx] = request;
    pp::MessageLoop::GetForMainThread().PostWork(callback, 0);
    do {
      pthread_cond_wait(&cond_, &mutex_);
    } while (!MapContainsKey(&inbox_, idx));
    // Got reply
    string reply = inbox_[idx];
    inbox_.erase(idx);
    return reply;
  }

  // Called from main thread
  void HandleReply(size_t idx, const std::string& reply) {
    ScopedPthreadLock lock(&mutex_);
    inbox_[idx] = reply;
    pthread_cond_broadcast(&cond_);
  }

 private:
  
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  size_t next_id_;
  
  std::map<size_t, std::string> outbox_;
  std::map<size_t, std::string> inbox_;
};
