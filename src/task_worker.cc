/*
    worker.cc
    libtq
    2022-11-10
    Push Chen
*/

/*
MIT License

Copyright (c) 2020 Push Chen

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

#include "task_worker.h"
#include <chrono>

namespace libtq {

/**
 * @brief Init a worker with the task queue.
 * @remarks throw runtime error when the queue is not validate
*/
worker::worker(eq_wt q, thread_attribute attr) : 
  thread(attr),
  related_eq_(q)
{
}

/**
 * @brief Stop and quit worker
*/
worker::~worker() {
  this->stop();
}

void worker::main() {
  std::lock_guard<std::mutex> running_guard(this->running_lock_);
  // start signal
  this->started_();

  while (this->is_validate()) {
    auto sq = related_eq_.lock();
    if (!sq) {
      break;
    }
    auto st = sq->wait((size_t)this->current_priority(), [this]() { return !this->is_validate(); });
    if (!st) {
      continue;
    }
    // this is normal state
    if (this->current_priority() == this->configed_priority()) {
      if ((int)this->current_priority() < st->prio) {
        // upgrade the thread priority
        this->change_priority((thread_priority)st->prio);
        this->adjust_prio_time_ = std::chrono::steady_clock::now();
      } // else do nothing, we don't need to downgrade the thread priority
        // to run the lower priority job
    } else {
      if (st->prio <= (size_t)this->configed_priority()) {
        // Overclocking the thread priority for at least 30 seconds
        auto kept_duration = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - this->adjust_prio_time_).count();
        if (kept_duration > 30) {
          this->change_priority(this->configed_priority());
        }
      }
    }
    st->i.begin_time = std::chrono::steady_clock::now();
    // invoke the task
    if (st->i.before) st->i.before(&st->i);
    if (st->i.t) st->i.t();
    st->i.end_time = std::chrono::steady_clock::now();
    if (st->i.after) st->i.after(&st->i);
  }
}

/**
 * @brief Stop current worker, will wait until current running task to be stopped
*/
void worker::stop() {
  if (this->is_validate()) {
    this->invalidate_();
    if (auto sq = related_eq_.lock()) {
      sq->break_waiter(this->id());
    }
  }
  // wait until get the running lock
  std::lock_guard<std::mutex> _(running_lock_);
}

} // namespace libtq

// Push Chen
