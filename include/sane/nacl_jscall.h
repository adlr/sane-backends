// copyright
#ifndef NACL_JSCALL_H__
#define NACL_JSCALL_H__

#include <map>
#include <string>

#include <pthread.h>

#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"

namespace scanley {

class SynchronousJavaScriptCaller : public pp::CompletionCallback {
 public:
  explicit SynchronousJavaScriptCaller(pp::Instance* instance);

  // Run on main thread
  void Run(int32_t result);

  // The main Call method, used by non-main thread to synchronously call JS
  std::string Call(const std::string& request);

  // Called from main thread
  void HandleReply(size_t idx, const std::string& reply);

 private:
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
  size_t next_id_;
  
  std::map<size_t, std::string> outbox_;
  std::map<size_t, std::string> inbox_;

  pp::Instance* pp_instance_;
};

}  // namespace scanley

#endif  // include guard
