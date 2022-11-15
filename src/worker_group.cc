/*
    worker_group.cpp
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

#include "worker_group.h"

namespace libtq {

/**
 * @brief Create a worker group with default 2 workers
*/
worker_group::worker_group(eq_wt q, unsigned int worker_count) : related_eq_(q) {
  for (unsigned int i = 0; i < worker_count; ++i) {
    this->increase_worker();
  }
}

/**
 * @brief Destroy the group
*/
worker_group::~worker_group() {
  // nothing to do
}

/**
 * @brief Get current worker group's worker count
*/
size_t worker_group::size() const {
  std::lock_guard<std::mutex> _(this->worker_lock_);
  return this->workers_.size();
}

/**
 * @brief Check if current thread is in the worker group
*/
bool worker_group::in_worker_grouop() const {
  std::lock_guard<std::mutex> _(this->worker_lock_);
  auto c_tid = std::this_thread::get_id();
  for (auto& w : this->workers_) {
    if (w->id() == c_tid) {
      return true;
    }
  }
  return false;
}

/**
 * @brief increase a worker
*/
void worker_group::increase_worker() {
  worker_st w = std::make_shared<worker>(this->related_eq_);
  std::lock_guard<std::mutex> _(this->worker_lock_);
  this->workers_.push_back(w);
  w->start();
}

/**
 * @brief remove a worker if still has
*/
void worker_group::decrease_worker() {
  std::lock_guard<std::mutex> _(this->worker_lock_);
  if (this->workers_.size() == 0) {
    return;
  }
  this->workers_.pop_back();
}

} // namespace libtq

// Push Chen
