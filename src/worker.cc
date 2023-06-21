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

#include "worker.h"
#include <chrono>

namespace libtq {

/**
 * @brief Init a worker with the task queue.
 * @remarks throw runtime error when the queue is not validate
*/
worker::worker(eq_wt q) :
  running_status_(false), related_eq_(q)
{
}

/**
 * @brief Stop and quit worker
*/
worker::~worker() {
  this->stop();
}

/**
 * @brief return if current worker is running
*/
bool worker::is_running() const {
  return this->running_status_;
}

/**
 * @brief Get current worker's thread id
*/
std::thread::id worker::id() const {
  return this->running_tid_;
}

/**
 * @brief Start current worker
*/
void worker::start() {
  if (this->is_running()) {
    return;
  }
  std::mutex l;
  std::shared_ptr<std::condition_variable> ptr_cv = std::make_shared<std::condition_variable>();
  std::thread t([ptr_cv, this]() {
    std::lock_guard<std::mutex> running_guard(this->running_lock_);
    this->running_tid_ = std::this_thread::get_id();
    this->running_status_ = true;
    ptr_cv->notify_one();
    this->entrance_point();
  });
  t.detach();
  while (true) {
    std::unique_lock<std::mutex> _(l);
    auto r = ptr_cv->wait_for(_, std::chrono::milliseconds(10), [this]() {
      return this->running_status_ == true;
    });
    if (r) {
      break;
    }
  }
}

/**
 * @brief Stop current worker, will wait until current running task to be stopped
*/
void worker::stop() {
  if (running_status_ != false) {
    running_status_ = false;
    if (auto sq = related_eq_.lock()) {
      sq->break_waiter(this->running_tid_);
    }
  }
  // wait until get the running lock
  std::lock_guard<std::mutex> _(running_lock_);
}

/**
 * @brief inner thread main function
*/
void worker::entrance_point() {
  while (running_status_) {
    auto sq = related_eq_.lock();
    if (!sq) {
      running_status_ = false;
      break;
    }
    auto st = sq->wait([this]() { return !this->running_status_; });
    if (!st) {
      continue;
    }
    st->i.btime = std::chrono::steady_clock::now();
    // invoke the task
    if (st->i.before) st->i.before(&st->i);
    if (st->i.t) st->i.t();
    if (st->i.after) st->i.after(&st->i);
  }
}

} // namespace libtq

// Push Chen
