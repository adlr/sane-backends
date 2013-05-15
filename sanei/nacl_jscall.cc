// copyright

#include "sane/nacl_jscall.h"

#include <string>
#include <map>
#include <utility>

#include "ppapi/cpp/message_loop.h"

#include "sane/nacl_util.h"

using std::string;
using std::map;
using std::pair;

namespace scanley {

SynchronousJavaScriptCaller::SynchronousJavaScriptCaller(
    pp::Instance* instance)
    : next_id_(0), pp_instance_(instance) {
  pthread_mutex_init(&mutex_, NULL);
}

// Run on main thread
void SynchronousJavaScriptCaller::Run(int32_t result) {
  ScopedPthreadLock lock(&mutex_);
  while (!outbox_.empty()) {
    const pair<size_t, string>& msg = *outbox_.begin();
    char buf[msg.second.size() + 20];
    int rc = snprintf(buf, sizeof(buf), "%zu:%s", msg.first, msg.second.c_str());
    if (rc <= 0) {
      fprintf(stderr, "snprintf failed");
      return;
    }
    outbox_.erase(outbox_.begin());

    ScopedPthreadUnlock unlock(&mutex_);

    pp::Var pp_msg = pp::Var(buf);
    pp_instance_->PostMessage(pp_msg);
  }
}

string SynchronousJavaScriptCaller::Call(const string& request) {
  ScopedPthreadLock lock(&mutex_);
  size_t idx = next_id_++;
  outbox_[idx] = request;
  pp::MessageLoop::GetForMainThread().PostWork(*this, 0);
  do {
    pthread_cond_wait(&cond_, &mutex_);
  } while (!MapContainsKey(inbox_, idx));
  // Got reply
  string reply = inbox_[idx];
  inbox_.erase(idx);
  return reply;
}

// Called from main thread
void SynchronousJavaScriptCaller::HandleReply(
    size_t idx, const std::string& reply) {
  ScopedPthreadLock lock(&mutex_);
  inbox_[idx] = reply;
  pthread_cond_broadcast(&cond_);
}

}  // namespace scanley
