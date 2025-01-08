/*
  task_thread.cc
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

#include "task_thread.h"

namespace libtq {

const char* _default_thread_name_(thread_priority prio) {
  if (prio == thread_priority::k_broken) {
    return "libtq_worker_pX";
  } else if (prio == thread_priority::k_low) {
    return "libtq_worker_p1";
  } else if (prio == thread_priority::k_normal) {
    return "libtq_worker_p2";
  } else if (prio == thread_priority::k_high) {
    return "libtq_worker_p3";
  } else if (prio == thread_priority::k_highest) {
    return "libtq_worker_p4";
  } else {
    return "libtq_worker_p5";
  }
}

/**
 * @brief Create thread attribute
*/
thread_attribute make_thread_attribute(unsigned int stack_size, void * stack_ptr, thread_priority priority, const char* name) {
  thread_attribute attr;
  attr.stack_size = stack_size;
  attr.stack_ptr = stack_ptr;
  attr.priority = priority;
  attr.name = name;
  return attr;
}
thread_attribute make_thread_attribute_with_priority(thread_priority priority) {
  return make_thread_attribute(k_thread_attribute_default_stack_size, nullptr, priority, _default_thread_name_(priority));
}
thread_attribute default_thread_attribute() {
  return make_thread_attribute_with_priority(thread_priority::k_normal);
}

thread::thread() : 
  attr_(default_thread_attribute()),
  joinable_(true),
  validate_(false),
  current_priority_(thread_priority::k_normal)
{
}
thread::thread(thread_attribute attr) :
  attr_(attr),
  joinable_(true),
  validate_(false),
  current_priority_(thread_priority::k_normal)
{
}

void thread::start() {
  if (this->validate_ == true) {
    return;
  }
  std::mutex init_lock;
  if (!this->create_thread_with_attr_()) {
    return;
  }
  while (true) {
    std::unique_lock<std::mutex> _(init_lock);
    auto r = init_cv_.wait_for(_, std::chrono::milliseconds(10), [this]() {
      return this->validate_ == true;
    });
    if (r) {
      break;
    }
  }
}

bool thread::is_validate() const {
  return this->validate_ == true;
}
thread_priority thread::configed_priority() const {
  return this->attr_.priority;
}

thread_priority thread::current_priority() const {
  return this->current_priority_;
}

std::thread::id thread::id() const {
  return this->thread_id_;
}
void thread::invalidate_() {
  this->validate_ = false;
}
void thread::started_() {
  this->init_cv_.notify_one();
}

} // namespace libtq

// Push Chen
