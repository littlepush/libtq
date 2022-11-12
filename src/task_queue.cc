/*
    task_queue.cc
    libtq
    2022-11-11
    Push Chen
*/

/*
MIT License

Copyright (c) 2019 Push Chen

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

#include "task_queue.h"

namespace libtq {

/**
 * @brief Post a async task
*/
void task_queue::post_task(task_location loc, task_t t) {
  std::weak_ptr<task_queue> weak_self = this->shared_from_this();
  task st;
  st.t = t;
  st.loc = loc;
  st.ptime = std::chrono::steady_clock::now();

  st.after = [weak_self](task* ptask) {
    if (auto self = weak_self.lock()) {
      std::lock_guard<std::mutex> _(self->tq_lock_);
      self->tq_.pop_front();
      if (self->tq_.size() > 0) {
        if (auto seq = self->related_eq_.lock()) {
          seq->emplace_back(std::move(self->tq_.front()));
        }
      }
    }
  };

  std::lock_guard<std::mutex> _(this->tq_lock_);
  this->tq_.emplace_back(std::move(st));
  // the only one in the queue is current task
  if (this->tq_.size() == 1) {
    if (auto seq = this->related_eq_.lock()) {
      seq->emplace_back(std::move(this->tq_.front()));
    }
  }
}

/**
 * @brief Wait for current task to be done
*/
void task_queue::sync_task(task_location loc, task_t t) {
  std::weak_ptr<task_queue> weak_self = this->shared_from_this();
  task st;
  st.t = t;
  st.loc = loc;
  st.ptime = std::chrono::steady_clock::now();

  
}

} // namespace libtq

// Push Chen
