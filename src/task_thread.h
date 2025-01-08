/*
  task_thread.h
  libtq
  2024-11-01
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

#pragma once

#ifndef LIBTQ_TASK_THREAD_H__
#define LIBTQ_TASK_THREAD_H__

#include <thread>
#include <atomic>
#include <condition_variable>
#include "task.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#if defined(APPLE)
#include <pthread_spis.h>
#endif
#endif

namespace libtq {

#if defined(_WIN32)
using thread_handler = HANDLE;
#else
using thread_handler = pthread_t;
#endif

enum {
  k_thread_attribute_default_stack_size = 512 * 1024  // 512K as the default stack size
};

enum class thread_priority : size_t {
  k_broken = 0,
  k_low = 1,
  k_normal = 2,
  k_high = 3,
  k_highest = 4,
  k_realtime = 5
};

struct thread_attribute {
  intptr_t          stack_size{k_thread_attribute_default_stack_size};
  void *            stack_ptr{nullptr};
  thread_priority   priority{thread_priority::k_normal};
  const char *      name{nullptr};
};

/**
 * @brief Create thread attribute
*/
thread_attribute make_thread_attribute(unsigned int stack_size, void * stack_ptr, thread_priority priority, const char* name);
thread_attribute make_thread_attribute_with_priority(thread_priority priority);
thread_attribute default_thread_attribute();

/**
 * @brief Internal thread object
*/
class thread {
public:
  thread();
  thread(thread_attribute attr);
  virtual ~thread();

public:
  virtual void main() = 0;

public:
  /**
   * @brief Start the thread
  */
  virtual void start();

  /**
   * @brief Detach current thread and can safe destroy current object
  */
  void detach();

  /**
   * @brief change the thread priority at runtime
  */
  void change_priority(thread_priority priority);

  /**
   * @brief If current thread is validate
  */
  bool is_validate() const;

  /**
   * @brief Get the priority setting in the attribute when creating current thread
  */
  thread_priority configed_priority() const;

  /**
   * @brief Get current priority the thread is
  */
  thread_priority current_priority() const;

  /**
   * @brief Get the thread id
  */
  std::thread::id id() const;

protected:
  thread(const thread&) = delete;
  thread(thread&&) = delete;
  thread& operator = (const thread&) = delete;
  thread& operator = (thread&&) = delete;

  /**
   * @brief Impl of thread creating
  */
  bool create_thread_with_attr_();

  /**
   * @brief Internal entrance
  */
  void entrance_();

  /**
   * @brief Force to change the validate flag to false
  */
  void invalidate_();

  /**
   * @brief Tell start state changed when running in main
  */
  void started_();

private:
  thread_handler  handler_;
  std::thread::id thread_id_;
  thread_attribute attr_;
  std::atomic<bool> joinable_;
  std::atomic<bool> validate_;
  std::atomic<thread_priority> current_priority_;
  std::condition_variable init_cv_;
};

} // namespace libtq


#endif

// Push Chen
