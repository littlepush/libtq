/*
    event_queue.h
    libtq
    2022-09-28
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

#pragma once

#ifndef LIBTQ_EVENT_QUEUE_H__
#define LIBTQ_EVENT_QUEUE_H__

#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <list>

namespace libtq {

/**
 * @brief Event Queue
*/
template< typename _Ty >
class event_queue {
public:
  /**
   * @brief C'str, 
  */
  event_queue();

  /**
   * @brief D'str, will force to break all waiting thread
  */
  ~event_queue() {
    this->break_queue();
  }

public:
  struct item_wrapper {
    item_wrapper(_Ty&& r) : i(std::move(r)) {}
    _Ty i;
  };
  typedef std::weak_ptr<item_wrapper>     item_weak_t;
  typedef std::shared_ptr<item_wrapper>   item_strong_t;
  typedef std::lock_guard<std::mutex>     eq_lg;
  typedef std::unique_lock<std::mutex>    eq_ul;

public:

  /**
   * @brief Block and wait for item, unless break the queue
   * @return shared ptr of the item, or nullptr
  */
  item_strong_t wait() {
    eq_ul ul(this->l_);
    cv_.wait(std::move(ul));
    if (il_.size() == 0) {
      return nullptr;
    }
    auto ti = il_.front();
    il_.pop_front();
    return ti;
  }

  /**
   * @brief Block and wait for item till timeout or queue has been broken
   * @return shared ptr of the item or nullptr
  */
  item_strong_t wait_for(std::chrono::nanoseconds timeout) {
    eq_ul ul(this->l_);
    auto ret = cv_.wait_for(std::move(ul), timeout);
    if (ret == std::cv_status::timeout || il_.size() == 0) {
      return nullptr;
    }
    auto ti = il_.front();
    il_.pop_front();
    return ti;
  }

public:
  /**
   * @brief Add item to the end of the queue
  */
  item_weak_t emplace_back(_Ty&& item) {
    item_strong_t i(new item_wrapper(std::move(item)));
    item_weak_t r = i;
    eq_lg lg(this->l_);
    il_.emplace_back(std::move(i));
    cv_.notify_one();
    return r;
  }

  /**
   * @brief Insert item to the beginning of the queue
  */
  item_weak_t emplace_front(_Ty&& item) {
    item_strong_t i(new item_wrapper(std::move(item)));
    item_weak_t r = i;
    eq_lg lg(this->l_);
    il_.emplace_front(std::move(i));
    cv_.notify_one();
    return r;
  }

  /**
   * @brief Cancel all pending item
  */
  void cancel_all() {
    eq_lg lg(this->l_);
    // clear all item
    il_.clear();
  }

  /**
   * @brief Cancel a specified item if it is still in queue
  */
  void cancel(item_weak_t i) {
    if (item_strong_t si = i.lock()) {
      eq_lg lg(this->l_);
      // Mark the item as invalidate
      auto it = il_.begin();
      for(; it != il_.end(); ++it) {
        if (it != si) continue;
        il_.erase(it);
        break;
      }
    }
  }

  /**
   * @brief Tell all waiting thread to stop
  */
  void break_queue() {
    eq_lg lg(this->l_);
    // clear all item
    il_.clear();
    cv_.notify_all();
  }

protected:
  /**
   * @brief Inner item storage
  */
  std::list<item_strong_t> il_;

  /**
   * @brief Mutex for the cv
  */
  std::mutex l_;

  /**
   * @brief CV to control event
  */
  std::condition_variable cv_;
};

} // namespace libtq

#endif

// Push Chen
