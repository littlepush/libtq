/*
  task_thread_posix.cc
  libtq
  2024-11-04
  Push Chen
*/

/*
MIT License

Copyright (c) 2023 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#if !defined(_WIN32)

#include "task_thread.h"

#if !defined(__APPLE__)
#include <sys/prctl.h>
#endif

namespace libtq {

#if defined(__APPLE__)
/*
 * These are forward-declarations for methods that are part of the
 * ObjC runtime. They are declared in the private header objc-internal.h.
 * These calls are what clang inserts when using @autoreleasepool in ObjC,
 * but here they are used directly in order to keep this file C++.
 * https://clang.llvm.org/docs/AutomaticReferenceCounting.html#runtime-support
 */
extern "C" {
void* objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void* pool);
}

namespace {
class ScopedAutoReleasePool {
public:
    ScopedAutoReleasePool() : pool_(objc_autoreleasePoolPush()) {}
    ~ScopedAutoReleasePool() { objc_autoreleasePoolPop(pool_); }

private:
    void* const pool_;
};
}  // namespace

#endif

struct __entrance__ : private thread {
  static void run(thread* t) {
    #if defined(__APPLE__)
    ScopedAutoReleasePool pool;
    #endif
    (t->*&__entrance__::entrance_)();
  }
};

void * __pthread_run_thread__(void * thread_param) {
  thread* pt = (thread *)thread_param;
  __entrance__::run(pt);
  return NULL;
}

void __change_thread_priority__(thread_handler handler, thread_priority priority) {
  int min_prio = sched_get_priority_min(SCHED_FIFO);
  int max_prio = sched_get_priority_max(SCHED_FIFO);
  if (min_prio == -1 || max_prio == -1) {
    return;
  }
  if (max_prio - min_prio <= 2) {
    return;
  }
  
  // Convert priority to system priorities:
  sched_param param;
  const int top_prio = std::max(max_prio - 1, min_prio);
  const int low_prio = std::min(min_prio + 1, max_prio);
  const int mid_prio = int((low_prio + top_prio - 1) / 2);
  switch (priority) {
    case thread_priority::k_low:
      param.sched_priority = low_prio;
      break;
    case thread_priority::k_normal:
      // in the middle
      param.sched_priority = mid_prio;
      break;
    case thread_priority::k_high:
      param.sched_priority = std::max(top_prio - 2, mid_prio);
      break;
    case thread_priority::k_highest:
      param.sched_priority = std::max(top_prio - 1, mid_prio);
      break;
    case thread_priority::k_realtime:
      param.sched_priority = top_prio;
      break;
    default:
      return;
  }
  (void)pthread_setschedparam(handler, SCHED_FIFO, &param);
}

thread::~thread() {
  if (this->joinable_) {
    (void)pthread_join(handler_, nullptr);
  }
}

bool thread::create_thread_with_attr_() {
  pthread_attr_t phattr;
  pthread_attr_init(&phattr);
  if (attr_.stack_ptr != nullptr) {
    (void)pthread_attr_setstack(&phattr, attr_.stack_ptr, attr_.stack_size);
  } else {
    pthread_attr_setstacksize(&phattr, attr_.stack_size);
  }
  int ret = pthread_create(&this->handler_, &phattr, &__pthread_run_thread__, this);
  pthread_attr_destroy(&phattr);
  return (ret == 0);
}

void thread::detach() {
  if (!this->validate_) {
    return;
  }
  if (!this->joinable_) {
    return;
  }
  if (pthread_detach(this->handler_) == 0) {
    this->joinable_ = false;
  }
}
void thread::change_priority(thread_priority priority) {
  if (this->thread_id_ != std::this_thread::get_id()) {
    return;
  }
  if (priority == thread_priority::k_broken) {
    priority = thread_priority::k_low;
  }
  if (priority == this->current_priority_) {
    return;
  }
  __change_thread_priority__(this->handler_, priority);
  this->current_priority_ = priority;
}

void thread::entrance_() {
  this->thread_id_ = std::this_thread::get_id();
  // Config the thread name and priority
  this->change_priority(this->attr_.priority);
  if (this->attr_.name != nullptr) {
    #if defined(__APPLE__)
    pthread_setname_np(this->attr_.name);
    #else
    prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(this->attr_.name));  // NOLINT
    #endif
  }
  this->validate_ = true;
  this->main();
  this->validate_ = false;
}

} // namespace libtq

#endif // not defined(_WIN32)
