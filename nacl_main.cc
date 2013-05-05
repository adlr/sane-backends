/// Copyright (c) 2012 The Native Client Authors. All rights reserved.
/// Use of this source code is governed by a BSD-style license that can be
/// found in the LICENSE file.
///
/// @file hello_tutorial.cc
/// This example demonstrates loading, running and scripting a very simple NaCl
/// module.  To load the NaCl module, the browser first looks for the
/// CreateModule() factory method (at the end of this file).  It calls
/// CreateModule() once to load the module code from your .nexe.  After the
/// .nexe code is loaded, CreateModule() is not called again.
///
/// Once the .nexe code is loaded, the browser than calls the CreateInstance()
/// method on the object returned by CreateModule().  It calls CreateInstance()
/// each time it encounters an <embed> tag that references your NaCl module.
///
/// The browser can talk to your NaCl module via the postMessage() Javascript
/// function.  When you call postMessage() on your NaCl module from the browser,
/// this becomes a call to the HandleMessage() method of your pp::Instance
/// subclass.  You can send messages back to the browser by calling the
/// PostMessage() method on your pp::Instance.  Note that these two methods
/// (postMessage() in Javascript and PostMessage() in C++) are asynchronous.
/// This means they return immediately - there is no waiting for the message
/// to be handled.  This has implications in your program design, particularly
/// when mutating property values that are exposed to both the browser and the
/// NaCl module.

#include <cstdio>
#include <fcntl.h>
#include <map>
#include <tr1/memory>
#include <pthread.h>
#include <stdarg.h>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <vector>

#include <sys/errno.h>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

#include "sane/sane.h"

using std::map;
using std::vector;
using std::tr1::shared_ptr;

namespace adlr {
class FakePipeManager;
}

/// The Instance class.  One of these exists for each instance of your NaCl
/// module on the web page.  The browser will ask the Module object to create
/// a new Instance for each occurence of the <embed> tag that has these
/// attributes:
///     type="application/x-nacl"
///     src="hello_tutorial.nmf"
/// To communicate with the browser, you must override HandleMessage() for
/// receiving messages from the browser, and use PostMessage() to send messages
/// back to the browser.  Note that this interface is asynchronous.
class HelloTutorialInstance : public pp::Instance {
 public:
  /// The constructor creates the plugin-side instance.
  /// @param[in] instance the handle to the browser-side plugin instance.
  explicit HelloTutorialInstance(PP_Instance instance) : pp::Instance(instance)
  {}
  virtual ~HelloTutorialInstance() {}


  void PostString(const std::string& str) {
    pp::Var var_reply = pp::Var(str);
    PostMessage(var_reply);
  }

  /// Handler for messages coming in from the browser via postMessage().  The
  /// @a var_message can contain anything: a JSON string; a string that encodes
  /// method names and arguments; etc.  For example, you could use
  /// JSON.stringify in the browser to create a message that contains a method
  /// name and some parameters, something like this:
  ///   var json_message = JSON.stringify({ "myMethod" : "3.14159" });
  ///   nacl_module.postMessage(json_message);
  /// On receipt of this message in @a var_message, you could parse the JSON to
  /// retrieve the method name, match it to a function call, and then call it
  /// with the parameter.
  /// @param[in] var_message The message posted by the browser.
  virtual void HandleMessage(const pp::Var& var_message) {
    printf("about to call sane_init\n");
    SANE_Status rc = sane_init(NULL, NULL);
    if (rc != SANE_STATUS_GOOD) {
      PostString("sane_init failed");
      return;
    }
    PostString("sane_init success2");
    const SANE_Device** device_list;
    rc = sane_get_devices(&device_list, SANE_TRUE);  // true = local only
    if (rc != SANE_STATUS_GOOD) {
      PostString("sane_get_devices failed");
      return;
    }
    for (size_t i = 0; device_list[i]; i++) {
      PostString("sane_get_devices returned a device");
      if (i > 4) {
        PostString("sane_get_devices returned a device (abort!)");
        return;
      }
    }
    PostMessage("done listing devices");
    // TODO(sdk_user): 1. Make this function handle the incoming message.
  }
};

/// The Module class.  The browser calls the CreateInstance() method to create
/// an instance of your NaCl module on the web page.  The browser creates a new
/// instance for each <embed> tag with type="application/x-nacl".
class HelloTutorialModule : public pp::Module {
 public:
  HelloTutorialModule() : pp::Module() {}
  virtual ~HelloTutorialModule() {}

  /// Create and return a HelloTutorialInstance object.
  /// @param[in] instance The browser-side instance.
  /// @return the plugin-side instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new HelloTutorialInstance(instance);
  }
};

namespace pp {
/// Factory function called by the browser when the module is first loaded.
/// The browser keeps a singleton of this module.  It calls the
/// CreateInstance() method on the object you return to make instances.  There
/// is one instance per <embed> tag on the page.  This is the main binding
/// point for your NaCl module with the browser.
Module* CreateModule() {
  return new HelloTutorialModule();
}
}  // namespace pp

namespace adlr {

const size_t kBufBytes = 64 * 1024;

class ScopedPthreadLock {
 public:
  explicit ScopedPthreadLock(pthread_mutex_t* mutex)
      : mutex_(mutex) {
    int rc = pthread_mutex_lock(mutex);
    if (rc)
      printf("pthread_mutex_lock: %d (ERR)\n", rc);
  }
  ~ScopedPthreadLock() {
    int rc = pthread_mutex_unlock(mutex_);
    if (rc)
      printf("pthread_mutex_unlock: %d (ERR)\n", rc);
  }
 private:
  pthread_mutex_t* mutex_;
};

class FakePipe {
 public:
  FakePipe();

  ssize_t Read(unsigned char* buf, size_t len);
  ssize_t Write(const unsigned char* buf, size_t len);
  void SetReadNonblock(bool nonblock) {
    ScopedPthreadLock lock(&mutex_);
    read_nonblock_ = nonblock;
  }
  void SetWriteNonblock(bool nonblock) {
    ScopedPthreadLock lock(&mutex_);
    write_nonblock_ = nonblock;
  }

 private:
  pthread_cond_t cond_;
  pthread_mutex_t mutex_;

  unsigned char buf_[kBufBytes];
  size_t size_;
  size_t next_read_;  // index into buf_

  size_t NextWrite() const { return (next_read_ + size_) % kBufBytes; }

  bool read_nonblock_:1;
  bool write_nonblock_:1;
};

FakePipe::FakePipe()
    : read_nonblock_(false), write_nonblock_(false) {
  pthread_cond_init(&cond_, NULL);
  pthread_mutex_init(&mutex_, NULL);
}

ssize_t FakePipe::Write(const unsigned char* buf, size_t len) {
  ScopedPthreadLock lock(&mutex_);
  for (size_t in_pos = 0; in_pos < len; in_pos++) {
    if (size_ < kBufBytes) {
      // Write another byte now
      buf_[NextWrite()] = buf[in_pos];
      size_++;

      // Kick other thread(s) incase they can read more now.
      int rc = pthread_cond_broadcast(&cond_);
      if (rc)
        printf("ERR: pthread_cond_broadcast: %d\n", rc);
    } else {
      // Can't write another byte now.
      if (write_nonblock_) {
        // Will return early
        if (in_pos > 0) {
          // We at least wrote some bytes. Return partial amount
          return in_pos;
        } else {
          errno = EAGAIN;
          return -1;
        }
      } else {
        // Time to block until there is space to write
        int rc = pthread_cond_wait(&cond_, &mutex_);
        if (rc)
          printf("ERR: (write) pthread_cond_wait: %d\n", rc);
      }
    }
  }
  return len;  // Full write success
}

ssize_t FakePipe::Read(unsigned char* buf, size_t len) {
  ScopedPthreadLock lock(&mutex_);
  // TODO(adlr): optimize
  for (size_t out_pos = 0; out_pos < len; out_pos++) {
    if (size_) {
      // Can read another byte now
      buf[out_pos] = buf_[next_read_];
      next_read_ = (next_read_ + 1) % kBufBytes;
      size_--;

      // Kick other thread(s) incase they can write more now.
      int rc = pthread_cond_broadcast(&cond_);
      if (rc)
        printf("ERR: pthread_cond_broadcast: %d\n", rc);
    } else {
      // Can't read another byte.
      if (read_nonblock_) {
        // Will return early
        if (out_pos > 0) {
          // We at least read some bytes. Return partial amount
          return out_pos;
        } else {
          errno = EAGAIN;
          return -1;
        }
      } else {
        // Time to block until there is data to read
        int rc = pthread_cond_wait(&cond_, &mutex_);
        if (rc)
          printf("ERR: pthread_cond_wait: %d\n", rc);
      }
    }
  }
  return len;  // full read success
}

template<typename Map, typename Key>
bool MapContainsKey(const Map& the_map, const Key& the_key) {
  return the_map.find(the_key) != the_map.end();
}

class FakePipeManager {
 public:
  FakePipeManager();
  int Pipe(int pipe_fd[2]);

  ssize_t Read(int fd, unsigned char* buf, size_t len);
  ssize_t Write(int fd, const unsigned char* buf, size_t len);
  int FcntlF_SETFL(int fd, long flags);

 private:
  pthread_mutex_t mutex_;
  // maps of fd->FakePipe
  map<int, shared_ptr<FakePipe> > readers_;
  map<int, shared_ptr<FakePipe> > writers_;
};

FakePipeManager::FakePipeManager() {
  pthread_mutex_init(&mutex_, NULL);
}

int FakePipeManager::Pipe(int pipe_fd[2]) {
  shared_ptr<FakePipe> new_pipe(new FakePipe);
  ScopedPthreadLock lock(&mutex_);
  
  vector<int> fds;

  // Find unused file descriptors
  for (int fd = 0; fds.size() < 2; fd++) {
    if (fd > 200) {
      printf("Pipe: ran out of fds\n");
      return -1;
    }
    if (MapContainsKey(readers_, fd) || MapContainsKey(writers_, fd))
      continue;
    fds.push_back(fd);
  }
  readers_[fds[0]] = new_pipe;
  writers_[fds[1]] = new_pipe;
  pipe_fd[0] = fds[0];
  pipe_fd[1] = fds[1];
}

ssize_t FakePipeManager::Read(int fd, unsigned char* buf, size_t len) {
  FakePipe* pipe = NULL;
  {
    ScopedPthreadLock lock(&mutex_);
    if (!MapContainsKey(readers_, fd)) {
      printf("no such fd for reading: %d\n", fd);
      return -1;
    }
    pipe = readers_[fd].get();
  }
  return pipe->Read(buf, len);
}

ssize_t FakePipeManager::Write(int fd, const unsigned char* buf, size_t len) {
  FakePipe* pipe = NULL;
  {
    ScopedPthreadLock lock(&mutex_);
    if (!MapContainsKey(writers_, fd)) {
      printf("no such fd for writing: %d\n", fd);
      return -1;
    }
    pipe = writers_[fd].get();
  }
  return pipe->Write(buf, len);
}

int FakePipeManager::FcntlF_SETFL(int fd, long flags) {
  FakePipe* pipe = NULL;
  bool is_reader = false;
  {
    ScopedPthreadLock lock(&mutex_);
    if (MapContainsKey(writers_, fd)) {
      pipe = writers_[fd].get();
    } else if (MapContainsKey(readers_, fd)) {
      pipe = readers_[fd].get();
      is_reader = true;
    } else {
      printf("Missing pipe for fcntl fd %d\n", fd);
      return -1;
    }
  }

  if (is_reader)
    pipe->SetReadNonblock((flags & O_NONBLOCK) != 0);
  else
    pipe->SetWriteNonblock((flags & O_NONBLOCK) != 0);

  return 0;
}

}  // namespace adlr

using adlr::FakePipeManager;
using adlr::ScopedPthreadLock;

namespace {

FakePipeManager* g_fake_pipe_manager = NULL;
pthread_mutex_t g_fake_pipe_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

}  // namespace {}

extern "C" {

  int pipe(int pipefd[2]) {
    {
      ScopedPthreadLock lock(&g_fake_pipe_manager_mutex);
      if (!g_fake_pipe_manager) {
        g_fake_pipe_manager = new FakePipeManager;
      }
    }
    return g_fake_pipe_manager->Pipe(pipefd);
  }

  ssize_t pipe_write(int fd, const void *buf, size_t count) {
    if (!g_fake_pipe_manager) {
      printf("No pipe manager!\n");
      return -1;
    }
    return g_fake_pipe_manager->Write(fd, reinterpret_cast<const unsigned char*>(buf), count);
  }

  ssize_t pipe_read(int fd, void *buf, size_t count) {
    if (!g_fake_pipe_manager) {
      printf("No pipe manager (read)!\n");
      return -1;
    }
    return g_fake_pipe_manager->Read(fd, reinterpret_cast<unsigned char*>(buf), count);
  }

  int fcntl(int fd, int cmd, ...) {
    switch(cmd) {
      case F_SETFL: {
        if (!g_fake_pipe_manager) {
          printf("No pipe manager (read)!\n");
          return -1;
        }
        long flags = 0;

        va_list ap;
        va_start(ap, cmd);
        flags = va_arg(ap, long);
        va_end(ap);
        return g_fake_pipe_manager->FcntlF_SETFL(fd, flags);
      }        
      default: {
        printf("Unhandled fcntl(fd=%d, cmd=%d)\n", fd, cmd);
        return -1;
      }
    }
  }

// Signal stubs
int sigpending(sigset_t *set) {
  return -1;  // error
}

  int sigmask(int signum) {
    return 0;
  }

  int sigaction(int signum, const struct sigaction *act,
                struct sigaction *oldact) {
    return -1;  // error
  }

  int sigsetmask(int mask) {
    return 0;
  }

  unsigned alarm(unsigned seconds) {
    return 0;
  }

  sighandler_t signal(int signum, sighandler_t handler) {
    return SIG_ERR;
  }

  int sigblock(int mask) {
    return 0;
  }

  int sanei_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    return -1;
  }
}
