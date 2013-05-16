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
void SynchronousJavaScriptCaller::StaticRun(void* self, int32_t unused) {
  SynchronousJavaScriptCaller* me = static_cast<SynchronousJavaScriptCaller*>(self);
  me->Run();
}

void SynchronousJavaScriptCaller::Run() {
  printf("JS Run() called\n");
  ScopedPthreadLock lock(&mutex_);
  printf("JS Run() got lock\n");
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
    printf("Posting message: %s\n", buf);
    pp_instance_->PostMessage(pp_msg);
  }
}

string SynchronousJavaScriptCaller::Call(const string& request) {
  printf("Entering Call(%s)\n", request.c_str());
  ScopedPthreadLock lock(&mutex_);
  size_t idx = next_id_++;
  printf("  lock acquired: %zu\n", idx);
  outbox_[idx] = request;

  pp::Module::Get()->core()->CallOnMainThread(
      0,
      pp::CompletionCallback(&StaticRun, this),
      PP_OK);


  //int rc = pp::MessageLoop::GetForMainThread().PostWork(*this, 0);
  //const char* rc_text = "unknown";
  //switch (rc) {
  //  case PP_OK: rc_text = "PP_OK"; break;
  //  case PP_ERROR_BADRESOURCE: rc_text = "PP_ERROR_BADRESOURCE"; break;
  //  case PP_ERROR_BADARGUMENT: rc_text = "PP_ERROR_BADARGUMENT"; break;
  //  case PP_ERROR_FAILED: rc_text = "PP_ERROR_FAILED"; break;
  //}
  //printf("PostWork returned %d %s\n", rc, rc_text);
  do {
    printf("waiting for work to be completed: %zu\n", idx);
    pthread_cond_wait(&cond_, &mutex_);
  } while (!MapContainsKey(inbox_, idx));
  // Got reply
  string reply = inbox_[idx];
  printf("got reply: [%s] %zu\n", reply.c_str(), idx);
  inbox_.erase(idx);
  return reply;
}

// Called from main thread
void SynchronousJavaScriptCaller::HandleReply(
    size_t idx, const std::string& reply) {
  printf("HandleReply(%s) called\n", reply.c_str());
  ScopedPthreadLock lock(&mutex_);
  inbox_[idx] = reply;
  pthread_cond_broadcast(&cond_);
}

}  // namespace scanley
