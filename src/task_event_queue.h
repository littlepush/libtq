/*
    task_event_queue.h
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

#ifndef LIBTQ_TASK_EVENT_QUEUE_H__
#define LIBTQ_TASK_EVENT_QUEUE_H__

#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <list>
#include <array>
#include <atomic>
#include <unordered_map>
#include <thread>

#if defined(_WIN32)
#pragma warning(disable: 4820)
#pragma warning(disable: 5045)
#endif

namespace libtq {

/**
 * @brief Event Queue
 * @param max_priority: max supported priority level in this queue, 0 = broken
*/
template< typename _Ty, size_t max_priority = 5, size_t normal_priority = 2 >
class event_queue {
public:
  /**
   * @brief C'str, 
  */
  event_queue() : st_(true), is_(0) {

  }
  event_queue(const event_queue&) = delete;
  event_queue(event_queue&&) = delete;
  event_queue& operator= (const event_queue&) = delete;
  event_queue& operator= (event_queue&&) = delete;

  /**
   * @brief D'str, will force to break all waiting thread
  */
  ~event_queue() {
    this->break_queue();
    while (pending_threads_.size() > 0) {
      std::this_thread::yield();
    }
  }

public:
  struct item_wrapper {
    item_wrapper(_Ty&& r, size_t p) : prio(p), i(std::move(r)) {}
    size_t prio;
    _Ty i;
  };
  typedef std::weak_ptr<item_wrapper>     item_weak_t;
  typedef std::shared_ptr<item_wrapper>   item_strong_t;
  typedef std::lock_guard<std::mutex>     eq_lg_t;
  typedef std::unique_lock<std::mutex>    eq_ul_t;

public:

  // void register_worker(size_t priority = normal_priority, int p_in) {
    
  // }

  /**
   * @brief Block and wait for item, unless break the queue
   * @return shared ptr of the item, or nullptr
  */
  item_strong_t wait(size_t priority = normal_priority, std::function<bool ()> pred = nullptr) {
    eq_ul_t ul(this->l_);
    auto tid = std::this_thread::get_id();
    pending_threads_[tid] = priority;
    while (!cv_.wait_for(ul, std::chrono::milliseconds(10), [this, tid, pred]() {
      auto tp = this->pending_threads_[tid];
      return (
        (tp != 0 && this->is_ > 0) ||  // current thread is alive and has pending item, means get signal
        (tp == 0) ||  // current thread is broken, need stop waiting
        (this->st_ == false) || // current queue has been broken, need stop waiting
        (pred && pred())
      );
    }));
    // cv_.wait(ul, [this, tid, pred]() {
    // });
    auto tp = pending_threads_[tid];
    pending_threads_.erase(tid);

    // Queue or thread has been broken, return nothing
    if (this->st_ == false || tp == 0) {
      return nullptr;
    }
    if (pred && pred()) {
      return nullptr;
    }
    // Will happen in a very low chance, when invoke
    // emplace and cancel_all in a very short time.
    if (is_ == 0) {
      return nullptr;
    }
    return this->pick_up_(priority);
  }

  /**
   * @brief Block and wait for item till timeout or queue has been broken
   * @return shared ptr of the item or nullptr
  */
  item_strong_t wait_for(std::chrono::nanoseconds timeout, size_t priority = normal_priority, std::function<bool ()> pred = nullptr) {
    eq_ul_t ul(this->l_);
    auto tid = std::this_thread::get_id();
    pending_threads_[tid] = priority;
    auto ret = cv_.wait_for(ul, timeout, [this, tid, pred]() {
      // same as wait
      auto pt = this->pending_threads_[tid];
      return (
        (pt != 0 && this->is_ > 0) ||
        (pt == 0) ||
        (this->st_ == false) || 
        (pred && pred())
      );
    });
    auto pt = pending_threads_[tid];
    pending_threads_.erase(tid);

    // Queue or thread has been broken, return nothing
    if (this->st_ == false || pt == 0) {
      return nullptr;
    }
    if (pred && pred()) {
      return nullptr;
    }
    // Will happen in a very low chance, when invoke
    // emplace and cancel_all in a very short time.
    if (this->is_ == 0) {
      return nullptr;
    }
    // timeout for current waiting oprand
    if (!ret) {
      return nullptr;
    }
    return this->pick_up_(priority);
  }

public:
  /**
   * @brief Add item to the end of the queue
  */
  item_weak_t emplace_back(_Ty&& item, size_t priority = normal_priority) {
    // a broken item should not be added to the queue
    if (priority <= 0) {
      return item_weak_t();
    }
    item_strong_t i(new item_wrapper(std::move(item), priority));
    item_weak_t r = i;
    eq_lg_t lg(this->l_);
    if (st_ == false) {
      return r;
    }
    il_[priority - 1].emplace_back(std::move(i));
    ++is_;
    cv_.notify_all();
    return r;
  }

  /**
   * @brief Insert item to the beginning of the queue
  */
  item_weak_t emplace_front(_Ty&& item, size_t priority = normal_priority) {
    // a broken item should not be added to the queue
    if (priority <= 0) return item_weak_t();
    item_strong_t i(new item_wrapper(std::move(item), priority));
    item_weak_t r = i;
    eq_lg_t lg(this->l_);
    if (st_ == false) {
      return r;
    }
    il_[priority - 1].emplace_front(std::move(i));
    ++is_;
    cv_.notify_all();
    return r;
  }

  /**
   * @brief Cancel all pending item
  */
  void cancel_all() {
    eq_lg_t lg(this->l_);
    // clear all item
    for (unsigned int i = 0; i < max_priority; ++i) {
      il_[i].clear();
    }
    is_ = 0;
  }

  /**
   * @brief Cancel a specified item if it is still in queue
  */
  void cancel(item_weak_t i) {
    if (item_strong_t si = i.lock()) {
      eq_lg_t lg(this->l_);
      // Mark the item as invalidate
      for (unsigned int idx = 0; idx < max_priority; ++idx) {
        auto it = il_[idx].begin();
        for(; it != il_[idx].end(); ++it) {
          if (*it != si) continue;
          il_[idx].erase(it);
          --is_;
          break;
        }
      }
    }
  }

  /**
   * @brief Break a waiting thread
  */
  void break_waiter(std::thread::id tid) {
    eq_lg_t lg(this->l_);
    auto t = pending_threads_.find(tid);
    if (t == pending_threads_.end()) {
      return;
    }
    t->second = 0;
    cv_.notify_all();
  }

  /**
   * @brief Return the waiting thread count
  */
  size_t waiter_count() const {
    eq_lg_t lg(this->l_);
    return pending_threads_.size();
  }

  /**
   * @brief Item count in queue
  */
  size_t pending_count() const {
    eq_lg_t lg(this->l_);
    return is_;
  }

protected:
  /**
   * @brief Tell all waiting thread to stop
  */
  void break_queue() {
    eq_lg_t lg(this->l_);
    this->st_ = false;
    // clear all item
    for (size_t i = 0; i < max_priority; ++i) {
      il_[i].clear();
    }
    is_ = 0;
    cv_.notify_all();
  }

  item_strong_t pick_up_(size_t t_prio) {
    size_t highest_prio = 0;
    size_t higher_waiter_count = 0;
    size_t higher_item_count = 0;
    for (auto& pd_th : pending_threads_) {
      if (pd_th.second > t_prio) {
        ++higher_waiter_count;
      }
    }
    for (size_t base_prio = (t_prio + 1); base_prio <= max_priority; ++base_prio) {
      higher_item_count += il_[base_prio - 1].size();
      if (il_[base_prio - 1].size() > 0) {
        highest_prio = base_prio;
      }
    }
    size_t pickup_prio = 0;
    if (
      (higher_item_count > 0 && higher_waiter_count == 0) || // no higher waiter wait for higher item
      (higher_item_count > higher_waiter_count * 2) // too many pending higher item
    ) {
      pickup_prio = highest_prio;
    } else {
      // pick the nearest prio item
      for (size_t base_prio = t_prio; base_prio > 0; --base_prio) {
        if (il_[base_prio - 1].size() > 0) {
          pickup_prio = base_prio;
          break;
        }
      }
      if (pickup_prio == 0) {
        // invalidate
        return nullptr;
      }
    }
    auto ti = il_[pickup_prio - 1].front();
    il_[pickup_prio - 1].pop_front();
    is_ -= 1;
    return ti;
  }

protected:
  /**
   * @brief Status of current queue
  */
  std::atomic_bool st_;
  /**
   * @brief Inner item storage
  */
  std::array<std::list<item_strong_t>, max_priority> il_;
  // std::list<item_strong_t> il_;
  /**
   * @brief Inner item size
  */
  size_t is_;
  /**
   * @brief Mutex for the cv
  */
  mutable std::mutex l_;

  /**
   * @brief CV to control event
  */
  std::condition_variable cv_;

  /**
   * @brief Pending threads cache
  */
  std::unordered_map<std::thread::id, size_t> pending_threads_;
};

} // namespace libtq

#endif

// Push Chen
